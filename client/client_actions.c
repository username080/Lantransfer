#include "client.h"
#include "../network/socket.h"
#include "../network/protocol.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

int start_client_send(Config *config, const char *local_path, int move) {
    int sockfd = create_client_socket(config->server_ip, config->port);
    if (sockfd < 0) {
        return -1;
    }
    printf("Connected to server %s:%d\n", config->server_ip, config->port);

    const char *base_name = strrchr(local_path, '/');
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

    if (move) {
        printf("Transfer successful. Deleting source path %s\n", local_path);
        remove_path(local_path);
    }

    printf("Send complete.\n");
    close(sockfd);
    return 0;
}

int start_client_get(Config *config, const char *remote_path, int move) {
    int sockfd = create_client_socket(config->server_ip, config->port);
    if (sockfd < 0) {
        return -1;
    }
    printf("Connected to server %s:%d\n", config->server_ip, config->port);

    uint8_t action = move ? ACTION_GET_MOVE : ACTION_GET;

    if (protocol_send_header(sockfd, action, remote_path) < 0) {
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

int start_client_exec(Config *config, const char *command_or_script, int detach) {
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

    uint8_t action = detach ? ACTION_EXEC_DETACH : ACTION_EXEC;

    if (protocol_send_header(sockfd, action, payload) < 0) {
        fprintf(stderr, "Failed to send exec header\n");
        close(sockfd);
        return -1;
    }

    if (detach) {
        // Just receive the Task ID / success message instantly and exit
        char buf[4096];
        ssize_t n = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = '\0';
            printf("%s", buf);
        }
        close(sockfd);
        return 0;
    }

    printf("Command sent. Waiting for output...\n");

    // Normal Attached Execution
    char buf[4096];
    ssize_t n;
    while ((n = recv(sockfd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        fprintf(stdout, "%s", buf);
        fflush(stdout);
    }

    printf("\nExecution complete.\n");
    close(sockfd);
    return 0;
}

int start_client_attach(Config *config, const char *task_id) {
    int sockfd = create_client_socket(config->server_ip, config->port);
    if (sockfd < 0) {
        return -1;
    }
    
    if (!task_id) {
        // No task ID provided, list all running tasks
        if (protocol_send_header(sockfd, ACTION_LIST_TASKS, "") < 0) {
            close(sockfd);
            return -1;
        }
        printf("Running Background Tasks:\n");
        printf("-------------------------\n");
    } else {
        // Task ID provided, attach to it
        if (protocol_send_header(sockfd, ACTION_ATTACH, task_id) < 0) {
            close(sockfd);
            return -1;
        }
        printf("Attaching to Task %s...\n", task_id);
    }
    
    char buf[4096];
    ssize_t n;
    while ((n = recv(sockfd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        fprintf(stdout, "%s", buf);
        fflush(stdout);
    }
    
    close(sockfd);
    return 0;
}

int start_client_read_log(Config *config, const char *task_id) {
    int sockfd = create_client_socket(config->server_ip, config->port);
    if (sockfd < 0) {
        return -1;
    }
    
    if (!task_id) {
        if (protocol_send_header(sockfd, ACTION_READ_LOG, "") < 0) {
            close(sockfd);
            return -1;
        }
    } else {
        if (protocol_send_header(sockfd, ACTION_READ_LOG, task_id) < 0) {
            close(sockfd);
            return -1;
        }
    }
    
    char buf[4096];
    ssize_t n;
    while ((n = recv(sockfd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        fprintf(stdout, "%s", buf);
        fflush(stdout);
    }
    
    close(sockfd);
    return 0;
}
