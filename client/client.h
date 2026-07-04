#ifndef CLIENT_H
#define CLIENT_H

#include "../core/config.h"

/**
 * ============================================================================
 * CLIENT ACTIONS INTERFACE
 * ============================================================================
 * These functions define everything a client can ask the server to do.
 */

/**
 * @brief Connects to the server and pushes a local file/folder.
 */
int start_client_send(Config *config, const char *local_path, int move);

/**
 * @brief Connects to the server and downloads a remote file/folder.
 */
int start_client_get(Config *config, const char *remote_path, int move);

/**
 * @brief Connects to the server and tells it to execute a script or command.
 */
int start_client_exec(Config *config, const char *command_or_script, int detach);

/**
 * @brief Connects to the server and attaches to a running background task.
 * If task_id is NULL, it lists all currently running tasks.
 */
int start_client_attach(Config *config, const char *task_id);

#endif // CLIENT_H
