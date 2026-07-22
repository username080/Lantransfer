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
#include <sys/wait.h>

/**
 * ============================================================================
 * LANTRANSFER ENTRY POINT
 * ============================================================================
 * This is the very first piece of code that runs when you type `./lantransfer`.
 * Its only job is to figure out what you want to do (Server vs Client) by 
 * looking at your `lantransfer.json` config, and then route you to the correct code.
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
    printf("\n\033[1;36mLanTransfer - High Performance Local File & Execution Engine\033[0m\n");
    printf("------------------------------------------------------------\n");
    
    printf("\033[1;33mSERVER COMMANDS:\033[0m\n");
    printf("  \033[1;32m%s\033[0m [-d|--daemon]                  Start the server (use -d to run in background)\n\n", prog_name);
    
    printf("\033[1;33mFILE TRANSFER COMMANDS:\033[0m\n");
    printf("  \033[1;32m%s send_file\033[0m <local_path> [-m]  Push a file/folder to the server (-m to move/delete original)\n", prog_name);
    printf("  \033[1;32m%s get_file\033[0m  <remote_path> [-m] Pull a file/folder from the server (-m to move/delete remote)\n\n", prog_name);
    
    printf("\033[1;33mEXECUTION COMMANDS:\033[0m\n");
    printf("  \033[1;32m%s exec_script\033[0m <script_path> [-r] Upload and run a script on the server (-r to detach to background)\n", prog_name);
    printf("  \033[1;32m%s attach\033[0m      [task_id]          View live output of a detached background script\n", prog_name);
    printf("  \033[1;32m%s read_log\033[0m    [task_id]          Read logs from completed or killed background scripts\n\n", prog_name);
    
    printf("\033[1;90mConfiguration loaded from 'lantransfer.json' in the current directory.\033[0m\n\n");
}

int main(int argc, char *argv[]) {
    // SECURITY PATCH: Prevent SIGPIPE from crashing the server when a client disconnects.
    signal(SIGPIPE, SIG_IGN);
    // AUTO-REAP ZOMBIES: Ensure any background tasks forked by the server don't become zombies.
    signal(SIGCHLD, SIG_IGN);
    
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
        
        if (run_daemon) {
            daemonize();
        }
        
        return start_server(&config) < 0 ? 1 : 0;
        
    // Route 2: Client Mode
    } else if (config.mode == MODE_CLIENT) {
        if (argc < 2) {
            print_usage(argv[0]);
            return 1;
        }

        const char *cmd = argv[1];

        // Route the client to the correct handler based on the command typed
        if (strcmp(cmd, "send_file") == 0) {
            if (argc < 3) { print_usage(argv[0]); return 1; }
            int move = (argc >= 4 && strcmp(argv[3], "-m") == 0);
            return start_client_send(&config, argv[2], move) < 0 ? 1 : 0;
            
        } else if (strcmp(cmd, "get_file") == 0) {
            if (argc < 3) { print_usage(argv[0]); return 1; }
            int move = (argc >= 4 && strcmp(argv[3], "-m") == 0);
            return start_client_get(&config, argv[2], move) < 0 ? 1 : 0;
            
        } else if (strcmp(cmd, "exec_script") == 0) {
            if (argc < 3) { print_usage(argv[0]); return 1; }
            int detach = (argc >= 4 && strcmp(argv[3], "-r") == 0);
            return start_client_exec(&config, argv[2], detach) < 0 ? 1 : 0;
            
        } else if (strcmp(cmd, "attach") == 0) {
            const char *task_id = (argc >= 3) ? argv[2] : NULL;
            return start_client_attach(&config, task_id) < 0 ? 1 : 0;
            
        } else if (strcmp(cmd, "read_log") == 0) {
            const char *task_id = (argc >= 3) ? argv[2] : NULL;
            return start_client_read_log(&config, task_id) < 0 ? 1 : 0;
            
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
