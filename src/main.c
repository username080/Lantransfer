#include "config.h"
#include "server.h"
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char *prog_name) {
    printf("Usage:\n");
    printf("  Server Mode: %s\n", prog_name);
    printf("  Client Send: %s send <local_path>\n", prog_name);
    printf("  Client Get:  %s get <remote_path>\n", prog_name);
    printf("Note: Configured via lantransfer.json in the current directory.\n");
}

int main(int argc, char *argv[]) {
    Config config;
    if (load_config("lantransfer.json", &config) < 0) {
        return 1;
    }

    if (config.mode == MODE_SERVER) {
        return start_server(&config) < 0 ? 1 : 0;
    } else if (config.mode == MODE_CLIENT) {
        if (argc < 3) {
            print_usage(argv[0]);
            return 1;
        }

        const char *cmd = argv[1];
        const char *path = argv[2];

        if (strcmp(cmd, "send") == 0) {
            return start_client_send(&config, path) < 0 ? 1 : 0;
        } else if (strcmp(cmd, "get") == 0) {
            return start_client_get(&config, path) < 0 ? 1 : 0;
        } else {
            fprintf(stderr, "Unknown client command: %s\n", cmd);
            print_usage(argv[0]);
            return 1;
        }
    } else {
        fprintf(stderr, "Unknown mode in lantransfer.json\n");
        return 1;
    }

    return 0;
}
