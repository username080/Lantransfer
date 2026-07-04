#include "client.h"
#include "socket.h"
#include "protocol.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

int start_client_send(Config *config, const char *local_path) {
    int sockfd = create_client_socket(config->server_ip, config->port);
    if (sockfd < 0) {
        return -1;
    }

    printf("Connected to server %s:%d\n", config->server_ip, config->port);

    char *base_name = strrchr(local_path, '/');
    if (base_name) {
        base_name++;
    } else {
        base_name = (char *)local_path;
    }

    if (protocol_send_header(sockfd, ACTION_SEND, base_name) < 0) {
        fprintf(stderr, "Failed to send header\n");
        close(sockfd);
        return -1;
    }

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

int start_client_get(Config *config, const char *remote_path) {
    int sockfd = create_client_socket(config->server_ip, config->port);
    if (sockfd < 0) {
        return -1;
    }

    printf("Connected to server %s:%d\n", config->server_ip, config->port);

    if (protocol_send_header(sockfd, ACTION_GET, remote_path) < 0) {
        fprintf(stderr, "Failed to send header\n");
        close(sockfd);
        return -1;
    }

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

    if (protocol_recv_stream(sockfd, dest_dir) < 0) {
        fprintf(stderr, "Failed to receive stream\n");
        close(sockfd);
        return -1;
    }

    printf("Receive complete.\n");
    close(sockfd);
    return 0;
}

int start_client_exec(Config *config, const char *command_or_script, int save_client, int save_server) {
    int sockfd = create_client_socket(config->server_ip, config->port);
    if (sockfd < 0) {
        return -1;
    }

    printf("Connected to server %s:%d\n", config->server_ip, config->port);

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
            payload_len = sizeof(payload) - 1;
        }
        memcpy(payload, command_or_script, payload_len);
    }
    payload[payload_len] = '\0';

    uint8_t action = save_server ? ACTION_EXEC_SAVE : ACTION_EXEC;

    if (protocol_send_header(sockfd, action, payload) < 0) {
        fprintf(stderr, "Failed to send exec header\n");
        close(sockfd);
        return -1;
    }

    printf("Command sent. Waiting for output...\n");

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

    char buf[4096];
    ssize_t n;
    while ((n = recv(sockfd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        fprintf(out, "%s", buf);
        fflush(out);
    }

    if (out != stdout) {
        fclose(out);
    }

    printf("\nExecution complete.\n");
    close(sockfd);
    return 0;
}
