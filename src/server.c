#include "server.h"
#include "socket.h"
#include "protocol.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>

static void handle_client(int client_fd, Config *config) {
    uint8_t action;
    char target_path[65536];

    if (protocol_recv_header(client_fd, &action, target_path, sizeof(target_path)) < 0) {
        fprintf(stderr, "Failed to receive request header\n");
        close(client_fd);
        return;
    }

    if (action == ACTION_SEND) {
        printf("Client is sending. Target path base: %s\n", target_path);

        char ts[64];
        get_timestamp_str(ts, sizeof(ts));

        char base_cache[4096];
        if (strlen(config->server_cache_dir) > 0) {
            make_absolute_path(base_cache, sizeof(base_cache), config->server_cache_dir);
        } else {
            snprintf(base_cache, sizeof(base_cache), "/home/%s/lantransfercache", config->username);
        }

        char dest_dir[8192];
        snprintf(dest_dir, sizeof(dest_dir), "%s/%s", base_cache, ts);

        printf("Saving to %s\n", dest_dir);
        mkdir_p(dest_dir);

        if (protocol_recv_stream(client_fd, dest_dir) < 0) {
            fprintf(stderr, "Error receiving stream\n");
        } else {
            printf("Stream received successfully.\n");
        }
    } else if (action == ACTION_GET) {
        printf("Client requested: %s\n", target_path);
        
        char *base_name = strrchr(target_path, '/');
        if (base_name) {
            base_name++; // skip slash
        } else {
            base_name = target_path;
        }

        if (protocol_send_path(client_fd, target_path, base_name) < 0) {
            fprintf(stderr, "Error sending path\n");
        } else {
            printf("Path sent successfully.\n");
        }
    } else if (action == ACTION_EXEC || action == ACTION_EXEC_SAVE) {
        printf("Client requested remote execution (Save: %s).\n", action == ACTION_EXEC_SAVE ? "yes" : "no");
        
        char base_cache[4096];
        if (strlen(config->server_cache_dir) > 0) {
            make_absolute_path(base_cache, sizeof(base_cache), config->server_cache_dir);
        } else {
            snprintf(base_cache, sizeof(base_cache), "/home/%s/lantransfercache", config->username);
        }
        mkdir_p(base_cache);
        setenv("LANTRANSFER_CACHE_DIR", base_cache, 1);

        char tmp_script[] = "/tmp/lantransfer_exec_XXXXXX";
        int fd = mkstemp(tmp_script);
        if (fd < 0) {
            perror("mkstemp");
        } else {
            // Write payload
            if (write(fd, target_path, strlen(target_path)) < 0) {
                perror("write");
            }
            close(fd);
            chmod(tmp_script, 0700);

            char cmd[512];
            snprintf(cmd, sizeof(cmd), "%s 2>&1", tmp_script);
            
            FILE *p = popen(cmd, "r");
            if (p) {
                if (action == ACTION_EXEC_SAVE) {
                    char ts[64];
                    get_timestamp_str(ts, sizeof(ts));


                    char out_path[8192];
                    snprintf(out_path, sizeof(out_path), "%s/exec_output_%s.log", base_cache, ts);
                    
                    FILE *out = fopen(out_path, "w");
                    if (out) {
                        char buf[4096];
                        size_t n;
                        while ((n = fread(buf, 1, sizeof(buf), p)) > 0) {
                            fwrite(buf, 1, n, out);
                        }
                        fclose(out);
                        char success_msg[16384];
                        snprintf(success_msg, sizeof(success_msg), "Output successfully saved on server at: %s\n", out_path);
                        send_all(client_fd, success_msg, strlen(success_msg));
                    } else {
                        char err[] = "Error opening server cache file for output\n";
                        send_all(client_fd, err, strlen(err));
                    }
                    pclose(p);
                } else {
                    char buf[4096];
                    size_t n;
                    while ((n = fread(buf, 1, sizeof(buf), p)) > 0) {
                        send_all(client_fd, buf, n);
                    }
                    pclose(p);
                }
            } else {
                char err[] = "Error executing command\n";
                send_all(client_fd, err, strlen(err));
            }
            unlink(tmp_script);
        }
    } else {
        fprintf(stderr, "Unknown action: %d\n", action);
    }

    close(client_fd);
}

int start_server(Config *config) {
    int server_fd = create_server_socket(config->port);
    if (server_fd < 0) {
        return -1;
    }

    printf("Server listening on port %d...\n", config->port);
    printf("Configured username: %s\n", config->username);

    // Ensure cache dir exists
    char cache_dir[4096];
    if (strlen(config->server_cache_dir) > 0) {
        make_absolute_path(cache_dir, sizeof(cache_dir), config->server_cache_dir);
    } else {
        snprintf(cache_dir, sizeof(cache_dir), "/home/%s/lantransfercache", config->username);
    }
    mkdir_p(cache_dir);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        printf("Accepted connection.\n");
        handle_client(client_fd, config);
    }

    close(server_fd);
    return 0;
}
