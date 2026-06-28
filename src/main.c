#include "config.h"
#include "server.h"
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static void daemonize(void) {
    pid_t pid;

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    if (setsid() < 0) exit(EXIT_FAILURE);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");

    int fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) {
            close(fd);
        }
    }
}

void print_usage(const char *prog_name) {
    printf("Usage:\n");
    printf("  Server Mode: %s [-d|--daemon]\n", prog_name);
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
        int run_daemon = 0;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--daemon") == 0) {
                run_daemon = 1;
                break;
            }
        }
        if (run_daemon) {
            daemonize();
        }
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
