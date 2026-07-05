#include "server.h"
#include "server_handlers.h"
#include "../network/socket.h"
#include "../network/protocol.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

/**
 * The Master Router for incoming connections.
 * This reads the very first byte (the Action byte) to see what the client
 * wants to do, and passes the socket off to the appropriate handler.
 */
static void handle_client(int client_fd, Config *config, char *base_cache) {
    // SECURITY/RELIABILITY: If the cache directory got deleted while the server
    // was running in the background, we instantly recreate it right before we use it!
    if (!is_directory(base_cache)) {
        ensure_server_cache_dir(config, base_cache, 4096);
    }

    uint8_t action;
    char target_path[65536];

    // Read the protocol handshake
    if (protocol_recv_header(client_fd, &action, target_path, sizeof(target_path)) < 0) {
        fprintf(stderr, "Failed to receive request header\n");
        close(client_fd);
        return;
    }

    // Abstract routing logic (Passing the pre-calculated base_cache!)
    if (action == ACTION_SEND) {
        handle_client_send(client_fd, target_path, base_cache);
        
    } else if (action == ACTION_GET) {
        handle_client_get(client_fd, target_path);
        
    } else if (action == ACTION_GET_MOVE) {
        handle_client_get_move(client_fd, target_path);
        
    } else if (action == ACTION_EXEC) {
        handle_client_exec(client_fd, target_path, 0, base_cache);
        
    } else if (action == ACTION_EXEC_DETACH) {
        handle_client_exec(client_fd, target_path, 1, base_cache);
        
    } else if (action == ACTION_LIST_TASKS) {
        handle_client_list_tasks(client_fd, base_cache);
        
    } else if (action == ACTION_ATTACH) {
        handle_client_attach(client_fd, target_path, base_cache);
        
    } else if (action == ACTION_READ_LOG) {
        handle_client_read_log(client_fd, target_path, base_cache);
        
    } else {
        fprintf(stderr, "Unknown action: %d\n", action);
    }

    // Disconnect when finished
    close(client_fd);
}

/**
 * Starts the master infinite loop.
 */
int start_server(Config *config) {
    // 1. Boot up the low-level TCP listening socket
    int server_fd = create_server_socket(config->port);
    if (server_fd < 0) {
        return -1;
    }

    printf("Server listening on port %d...\n", config->port);
    printf("Configured username: %s\n", config->username);

    // 2. Proactively ensure the Cache Directory actually exists on disk
    char base_cache[4096];
    ensure_server_cache_dir(config, base_cache, sizeof(base_cache));

    // 3. The Infinite Loop
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // This command blocks forever until a client tries to connect!
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd < 0) {
            perror("accept");
            continue; // Ignore broken handshakes and keep listening
        }

        printf("Accepted connection.\n");
        handle_client(client_fd, config, base_cache);
    }

    close(server_fd);
    return 0;
}
