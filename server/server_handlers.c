#include "server_handlers.h"
#include "../network/socket.h"
#include "../network/protocol.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

void handle_client_send(int client_fd, const char *target_path, const char *base_cache) {
    printf("Client is sending. Target path base: %s\n", target_path);

    char ts[64];
    get_timestamp_str(ts, sizeof(ts));

    char dest_dir[8192];
    snprintf(dest_dir, sizeof(dest_dir), "%s/%s", base_cache, ts);

    printf("Saving to %s\n", dest_dir);
    mkdir_p(dest_dir);

    if (protocol_recv_stream(client_fd, dest_dir) < 0) {
        fprintf(stderr, "Error receiving stream\n");
    } else {
        printf("Stream received successfully.\n");
    }
}

void handle_client_get(int client_fd, const char *target_path) {
    printf("Client requested GET: %s\n", target_path);
    char *base_name = strrchr(target_path, '/');
    if (base_name) base_name++; else base_name = (char *)target_path;

    if (protocol_send_path(client_fd, target_path, base_name) < 0) {
        fprintf(stderr, "Error sending path\n");
    } else {
        printf("Path sent successfully.\n");
    }
}

void handle_client_get_move(int client_fd, const char *target_path) {
    printf("Client requested GET MOVE: %s\n", target_path);
    char *base_name = strrchr(target_path, '/');
    if (base_name) base_name++; else base_name = (char *)target_path;

    if (protocol_send_path(client_fd, target_path, base_name) == 0) {
        printf("Transfer successful. Deleting source path %s\n", target_path);
        remove_path(target_path);
    } else {
        fprintf(stderr, "Error sending path during move.\n");
    }
}

void handle_client_list_tasks(int client_fd, const char *base_cache) {
    char file_path[4096];
    snprintf(file_path, sizeof(file_path), "%s/running_tasks.txt", base_cache);
    FILE *f = fopen(file_path, "r");
    if (!f) {
        char err[] = "No background tasks currently running.\n";
        send_all(client_fd, err, strlen(err));
        return;
    }
    
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        send_all(client_fd, buf, n);
    }
    fclose(f);
}

void handle_client_attach(int client_fd, const char *task_id, const char *base_cache) {
    char out_path[8192];
    snprintf(out_path, sizeof(out_path), "%s/exec_output_%s.log", base_cache, task_id);
    
    FILE *f = fopen(out_path, "r");
    if (!f) {
        char err[] = "Error: Could not find log file for that Task ID.\n";
        send_all(client_fd, err, strlen(err));
        return;
    }
    
    char tasks_path[4096];
    snprintf(tasks_path, sizeof(tasks_path), "%s/running_tasks.txt", base_cache);
    
    while (1) {
        char buf[4096];
        size_t n = fread(buf, 1, sizeof(buf), f);
        if (n > 0) {
            if (send_all(client_fd, buf, n) < 0) {
                break; // Client disconnected
            }
        } else {
            // EOF reached. Is the process still running?
            int is_running = 0;
            FILE *tf = fopen(tasks_path, "r");
            if (tf) {
                char line[4096];
                while (fgets(line, sizeof(line), tf)) {
                    if (strncmp(line, task_id, strlen(task_id)) == 0) {
                        is_running = 1;
                        break;
                    }
                }
                fclose(tf);
            }
            
            if (is_running) {
                usleep(100000); // 100ms sleep
                clearerr(f);    // Clear EOF to allow tailing
            } else {
                // Task is done. Try reading one last time to avoid race condition
                n = fread(buf, 1, sizeof(buf), f);
                if (n > 0) {
                    send_all(client_fd, buf, n);
                }
                break;
            }
        }
    }
    
    fclose(f);
}

void handle_client_exec(int client_fd, const char *target_path, int detach, const char *base_cache) {
    printf("Client requested remote execution (Detach: %s).\n", detach ? "yes" : "no");
    setenv("LANTRANSFER_CACHE_DIR", base_cache, 1);
    
    // QUALITY OF LIFE: Force Python scripts to live-stream their `print()` statements
    // instantly, instead of block-buffering them when running natively inside our C pipe!
    setenv("PYTHONUNBUFFERED", "1", 1);

    char tmp_script[] = "/tmp/lantransfer_exec_XXXXXX";
    int fd = mkstemp(tmp_script);
    if (fd < 0) { perror("mkstemp"); return; }
    if (write(fd, target_path, strlen(target_path)) < 0) { perror("write"); }
    close(fd);
    chmod(tmp_script, 0700);

    char ts[64];
    get_timestamp_str(ts, sizeof(ts));
    
    if (detach) {
        add_running_task(base_cache, ts, "Remote Script");
        
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            unlink(tmp_script);
            remove_running_task(base_cache, ts);
            return;
        }
        
        if (pid > 0) {
            // Parent returns instantly!
            char success_msg[4096];
            snprintf(success_msg, sizeof(success_msg), "Task detached successfully! Task ID: %s\nRun 'lantransfer attach %s' to view live output.\n", ts, ts);
            send_all(client_fd, success_msg, strlen(success_msg));
            // DO NOT UNLINK HERE! Let the child do it when it finishes.
            return; 
        }
        
        // Child Process
        close(client_fd); // Sever tie with client
        
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "%s 2>&1", tmp_script);
        FILE *p = popen(cmd, "r");
        if (p) {
            char out_path[8192];
            snprintf(out_path, sizeof(out_path), "%s/exec_output_%s.log", base_cache, ts);
            FILE *out = fopen(out_path, "w");
            if (out) {
                char buf[4096];
                size_t n;
                while ((n = fread(buf, 1, sizeof(buf), p)) > 0) {
                    fwrite(buf, 1, n, out);
                    fflush(out);
                }
                fclose(out);
            }
            pclose(p);
        }
        remove_running_task(base_cache, ts);
        unlink(tmp_script);
        exit(0); // Child must exit!
        
    } else {
        // Normal attached mode. ALWAYS save to server log.
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "%s 2>&1", tmp_script);
        FILE *p = popen(cmd, "r");
        if (p) {
            char out_path[8192];
            snprintf(out_path, sizeof(out_path), "%s/exec_output_%s.log", base_cache, ts);
            FILE *out = fopen(out_path, "w");
            if (out) {
                char buf[4096];
                size_t n;
                while ((n = fread(buf, 1, sizeof(buf), p)) > 0) {
                    fwrite(buf, 1, n, out);
                    fflush(out);
                    
                    if (client_fd != -1) {
                        if (send_all(client_fd, buf, n) < 0) {
                            // Connection Recovery: Network dropped!
                            // By explicitly not breaking the while loop, we seamlessly
                            // continue fetching popen output and writing to `out_path`!
                            printf("Client disconnected. Recovering to %s\n", out_path);
                            close(client_fd);
                            client_fd = -1; 
                        }
                    }
                }
                fclose(out);
                
                if (client_fd != -1) {
                    char success_msg[16384];
                    snprintf(success_msg, sizeof(success_msg), "\n--- Execution complete ---\nOutput successfully saved on server at: %s\n", out_path);
                    send_all(client_fd, success_msg, strlen(success_msg));
                }
            }
            pclose(p);
        } else {
            char err[] = "Error executing command\n";
            if (client_fd != -1) send_all(client_fd, err, strlen(err));
        }
        unlink(tmp_script);
    }
}
