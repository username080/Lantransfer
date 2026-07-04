#include "client.h"
#include "../network/socket.h"
#include "../network/protocol.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

/**
 * Pushes a file or folder from your laptop to the remote server.
 */
int start_client_send(Config *config, const char *local_path) {
    // 1. Establish the TCP connection to the server
    int sockfd = create_client_socket(config->server_ip, config->port);
    if (sockfd < 0) {
        return -1;
    }
    printf("Connected to server %s:%d\n", config->server_ip, config->port);

    // 2. Parse out the base name (e.g. "/var/log/syslog" -> "syslog")
    char *base_name = strrchr(local_path, '/');
    if (base_name) {
        base_name++;
    } else {
        base_name = (char *)local_path;
    }

    // 3. Send the ACTION_SEND handshake to the server
    if (protocol_send_header(sockfd, ACTION_SEND, base_name) < 0) {
        fprintf(stderr, "Failed to send header\n");
        close(sockfd);
        return -1;
    }

    // 4. Begin streaming the binary data over the network!
    printf("Sending %s...\n", local_path);
    if (protocol_send_path(sockfd, local_path, base_name) < 0) {
        fprintf(stderr, "Failed to send path\n");
        close(sockfd);
        return -1;
    }

    printf("Send complete.\n");
    close(sockfd);
    return 0;
}

/**
 * Downloads a file or folder from the remote server to your laptop.
 */
int start_client_get(Config *config, const char *remote_path) {
    // 1. Establish connection
    int sockfd = create_client_socket(config->server_ip, config->port);
    if (sockfd < 0) {
        return -1;
    }
    printf("Connected to server %s:%d\n", config->server_ip, config->port);

    // 2. Send the ACTION_GET handshake with the requested path
    if (protocol_send_header(sockfd, ACTION_GET, remote_path) < 0) {
        fprintf(stderr, "Failed to send header\n");
        close(sockfd);
        return -1;
    }

    // 3. Prepare the local disk to receive the data stream
    char ts[64];
    get_timestamp_str(ts, sizeof(ts));

    char base_cache[4096];
    if (strlen(config->client_cache_dir) > 0) {
        make_absolute_path(base_cache, sizeof(base_cache), config->client_cache_dir);
    } else {
        make_absolute_path(base_cache, sizeof(base_cache), ".");
    }

    char dest_dir[8192];
    snprintf(dest_dir, sizeof(dest_dir), "%s/%s", base_cache, ts);

    printf("Receiving files to %s\n", dest_dir);
    mkdir_p(dest_dir);

    // 4. Receive and reconstruct the files from the binary network stream
    if (protocol_recv_stream(sockfd, dest_dir) < 0) {
        fprintf(stderr, "Failed to receive stream\n");
        close(sockfd);
        return -1;
    }

    printf("Receive complete.\n");
    close(sockfd);
    return 0;
}

/**
 * Beams a raw bash/python script to the server and tells the server to execute it natively.
 */
int start_client_exec(Config *config, const char *command_or_script, int save_client, int save_server) {
    // 1. Establish connection
    int sockfd = create_client_socket(config->server_ip, config->port);
    if (sockfd < 0) {
        return -1;
    }
    printf("Connected to server %s:%d\n", config->server_ip, config->port);

    // 2. Prepare the Payload
    // If the user provided a file path (like ./script.py), we read the whole file into memory.
    // Otherwise, we assume they just typed a raw bash command (like "ls -la").
    char payload[65535];
    size_t payload_len = 0;
    
    FILE *f = fopen(command_or_script, "r");
    if (f) {
        printf("Reading script file %s...\n", command_or_script);
        payload_len = fread(payload, 1, sizeof(payload) - 1, f);
        fclose(f);
    } else {
        printf("Sending command string...\n");
        payload_len = strlen(command_or_script);
        if (payload_len >= sizeof(payload)) {
            payload_len = sizeof(payload) - 1; // Prevent overflow
        }
        memcpy(payload, command_or_script, payload_len);
    }
    payload[payload_len] = '\0';

    // 3. Determine if we are just executing, or executing AND saving a permanent log on the server
    uint8_t action = save_server ? ACTION_EXEC_SAVE : ACTION_EXEC;

    // 4. Send the handshake + payload
    if (protocol_send_header(sockfd, action, payload) < 0) {
        fprintf(stderr, "Failed to send exec header\n");
        close(sockfd);
        return -1;
    }

    printf("Command sent. Waiting for output...\n");

    // 5. Setup client-side log saving if the user requested the `-c` flag
    FILE *out = stdout;
    if (save_client && !save_server) {
        char ts[64];
        get_timestamp_str(ts, sizeof(ts));
        
        char base_cache[4096];
        if (strlen(config->client_cache_dir) > 0) {
            make_absolute_path(base_cache, sizeof(base_cache), config->client_cache_dir);
        } else {
            make_absolute_path(base_cache, sizeof(base_cache), ".");
        }
        mkdir_p(base_cache);

        char out_path[8192];
        snprintf(out_path, sizeof(out_path), "%s/exec_output_%s.log", base_cache, ts);
        
        out = fopen(out_path, "w");
        if (!out) {
            perror("fopen cache");
            out = stdout;
        } else {
            printf("Saving output to %s...\n", out_path);
        }
    }

    // 6. Listen to the Live Stream
    // The server will instantly push `stdout` chunks over the network as the script runs.
    char buf[4096];
    ssize_t n;
    while ((n = recv(sockfd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        fprintf(out, "%s", buf); // Print to screen (or client log file)
        fflush(out);
    }

    if (out != stdout) {
        fclose(out);
    }

    printf("\nExecution complete.\n");
    close(sockfd);
    return 0;
}
