#include "server_handlers.h"
#include "../network/socket.h"
#include "../network/protocol.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/**
 * Executes when a client pushes files to us.
 * We generate a timestamped folder inside our cache directory and 
 * tell the protocol parser to dump the binary stream directly into it.
 */
void handle_client_send(int client_fd, const char *target_path, Config *config) {
    printf("Client is sending. Target path base: %s\n", target_path);

    // 1. Generate a clean timestamp to prevent overwriting old transfers
    char ts[64];
    get_timestamp_str(ts, sizeof(ts));

    // 2. Resolve exactly where the server cache is located
    char base_cache[4096];
    if (strlen(config->server_cache_dir) > 0) {
        make_absolute_path(base_cache, sizeof(base_cache), config->server_cache_dir);
    } else {
        snprintf(base_cache, sizeof(base_cache), "/home/%s/lantransfercache", config->username);
    }

    // 3. Create the final destination folder
    char dest_dir[8192];
    snprintf(dest_dir, sizeof(dest_dir), "%s/%s", base_cache, ts);

    printf("Saving to %s\n", dest_dir);
    mkdir_p(dest_dir);

    // 4. Hand off the socket to the binary protocol stream decoder
    if (protocol_recv_stream(client_fd, dest_dir) < 0) {
        fprintf(stderr, "Error receiving stream\n");
    } else {
        printf("Stream received successfully.\n");
    }
}

/**
 * Executes when a client requests to download a file/folder from us.
 * We resolve the path they asked for and pipe it through the binary protocol.
 */
void handle_client_get(int client_fd, const char *target_path, Config *config) {
    // Silence unused warning for config since GET just pulls from target_path
    (void)config; 
    
    printf("Client requested: %s\n", target_path);
    
    // Find the actual file/folder name (ignoring parent directories)
    char *base_name = strrchr(target_path, '/');
    if (base_name) {
        base_name++; // skip the slash
    } else {
        base_name = (char *)target_path;
    }

    // Stream it over the network!
    if (protocol_send_path(client_fd, target_path, base_name) < 0) {
        fprintf(stderr, "Error sending path\n");
    } else {
        printf("Path sent successfully.\n");
    }
}

/**
 * Executes when a client wants us to run a bash or python script natively.
 * It writes the payload to a secure temp file, executes it via `popen()`, 
 * and handles live-streaming and saving the logs.
 */
void handle_client_exec(int client_fd, const char *target_path, int save_to_server, Config *config) {
    printf("Client requested remote execution (Save: %s).\n", save_to_server ? "yes" : "no");
    
    // 1. Resolve and create the server cache directory
    char base_cache[4096];
    if (strlen(config->server_cache_dir) > 0) {
        make_absolute_path(base_cache, sizeof(base_cache), config->server_cache_dir);
    } else {
        snprintf(base_cache, sizeof(base_cache), "/home/%s/lantransfercache", config->username);
    }
    mkdir_p(base_cache);
    
    // 2. IMPORTANT: Inject the cache path as an Environment Variable!
    // This allows any executed Python/Bash script to natively know where to save datasets.
    setenv("LANTRANSFER_CACHE_DIR", base_cache, 1);

    // 3. Create a highly secure, randomized temporary file for the script
    char tmp_script[] = "/tmp/lantransfer_exec_XXXXXX";
    int fd = mkstemp(tmp_script);
    if (fd < 0) {
        perror("mkstemp");
        return;
    }

    // 4. Write the raw string payload (the script contents) into the file
    if (write(fd, target_path, strlen(target_path)) < 0) {
        perror("write");
    }
    close(fd);
    
    // 5. Make it executable
    chmod(tmp_script, 0700);

    // 6. Execute it! We use 2>&1 to merge standard error into standard output.
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "%s 2>&1", tmp_script);
    
    FILE *p = popen(cmd, "r");
    if (p) {
        if (save_to_server) {
            // SCENARIO A: Save AND Live Stream
            char ts[64];
            get_timestamp_str(ts, sizeof(ts));

            char out_path[8192];
            snprintf(out_path, sizeof(out_path), "%s/exec_output_%s.log", base_cache, ts);
            
            FILE *out = fopen(out_path, "w");
            if (out) {
                char buf[4096];
                size_t n;
                // Loop endlessly as long as the script is printing output
                while ((n = fread(buf, 1, sizeof(buf), p)) > 0) {
                    fwrite(buf, 1, n, out);      // Write securely to disk
                    send_all(client_fd, buf, n); // Stream instantly to client
                }
                fclose(out);
                
                // Final success tag
                char success_msg[16384];
                snprintf(success_msg, sizeof(success_msg), "\n--- Execution complete ---\nOutput successfully saved on server at: %s\n", out_path);
                send_all(client_fd, success_msg, strlen(success_msg));
            } else {
                char err[] = "Error opening server cache file for output\n";
                send_all(client_fd, err, strlen(err));
            }
            pclose(p);
        } else {
            // SCENARIO B: Just stream output (Don't save)
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
    
    // 7. Cleanup the temporary script so we don't leak files
    unlink(tmp_script);
}
