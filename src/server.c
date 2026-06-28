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

static void handle_client(int client_fd, Config *config) {
    uint8_t action;
    char target_path[4096];

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
            snprintf(base_cache, sizeof(base_cache), "/home/%s/Lantransfercache", config->username);
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
        snprintf(cache_dir, sizeof(cache_dir), "/home/%s/Lantransfercache", config->username);
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
