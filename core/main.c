#include "config.h"
#include "../server/server.h"
#include "../client/client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

/**
 * ============================================================================
 * LANTRANSFER ENTRY POINT
 * ============================================================================
 * This is the very first piece of code that runs when you type `./lantransfer`.
 * Its only job is to figure out what you want to do (Server vs Client) by 
 * looking at your `lantransfer.json` config, and then route you to the correct code.
 */

/**
 * @brief Daemonization is the Unix magic that lets a program run in the background.
 * If you run the server with `-d`, this function essentially detaches the 
 * program from your terminal so it keeps running even if you close SSH.
 */
static void daemonize(void) {
    pid_t pid;

    // 1. Fork the process and let the parent die. This detaches us from the terminal.
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    // 2. Create a new session so we are our own group leader
    if (setsid() < 0) exit(EXIT_FAILURE);

    // 3. Fork AGAIN. This ensures we can never accidentally re-acquire a terminal.
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    // 4. Reset file permissions and move to the root directory
    umask(0);
    chdir("/");

    // 5. Silence all output by redirecting stdout/stderr into the void (/dev/null)
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

/**
 * @brief Prints the helpful instruction manual if you type the command wrong.
 */
void print_usage(const char *prog_name) {
    printf("Usage:\n");
    printf("  Server Mode: %s [-d|--daemon]\n", prog_name);
    printf("  Client Send: %s send_file <local_path>\n", prog_name);
    printf("  Client Get:  %s get_file <remote_path>\n", prog_name);
    printf("  Client Exec: %s exec_command <command> [-c|-s]\n", prog_name);
    printf("  Client Exec: %s exec_script <script_path> [-c|-s]\n", prog_name);
    printf("Note: Configured via lantransfer.json in the current directory.\n");
}

int main(int argc, char *argv[]) {
    // SECURITY PATCH: If a client abruptly drops connection during a live stream, 
    // Linux will send a SIGPIPE signal. If we don't explicitly ignore it, 
    // the OS will instantly kill our server!
    signal(SIGPIPE, SIG_IGN);
    
    // Load the JSON configuration
    Config config;
    if (load_config("lantransfer.json", &config) < 0) {
        return 1;
    }

    // Route 1: Server Mode
    if (config.mode == MODE_SERVER) {
        int run_daemon = 0;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--daemon") == 0) {
                run_daemon = 1;
                break;
            }
        }
        
        // If the user requested background mode, execute the Unix daemon magic
        if (run_daemon) {
            daemonize();
        }
        
        // Launch the infinite server loop!
        return start_server(&config) < 0 ? 1 : 0;
        
    // Route 2: Client Mode
    } else if (config.mode == MODE_CLIENT) {
        if (argc < 3) {
            print_usage(argv[0]);
            return 1;
        }

        const char *cmd = argv[1];
        const char *path = argv[2];

        // Route the client to the correct handler based on the command typed
        if (strcmp(cmd, "send_file") == 0) {
            return start_client_send(&config, path) < 0 ? 1 : 0;
            
        } else if (strcmp(cmd, "get_file") == 0) {
            return start_client_get(&config, path) < 0 ? 1 : 0;
            
        } else if (strcmp(cmd, "exec_command") == 0 || strcmp(cmd, "exec_script") == 0) {
            // Check for optional flags like "-s" (save on server)
            int save_client = 0;
            int save_server = 0;
            if (argc >= 4) {
                if (strcmp(argv[3], "-c") == 0) save_client = 1;
                else if (strcmp(argv[3], "-s") == 0) save_server = 1;
            }
            return start_client_exec(&config, path, save_client, save_server) < 0 ? 1 : 0;
            
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
