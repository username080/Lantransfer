#ifndef CLIENT_H
#define CLIENT_H

#include "config.h"

/**
 * Starts the client mode to send a file/directory.
 */
int start_client_send(Config *config, const char *local_path);

/**
 * Starts the client mode to get a file/directory.
 */
int start_client_get(Config *config, const char *remote_path);

/**
 * Starts the client mode to execute a command or script.
 * @param save_client If true, saves stream output to client_cache_dir instead of printing.
 * @param save_server If true, requests the server to save the output in its server_cache_dir.
 */
int start_client_exec(Config *config, const char *command_or_script, int save_client, int save_server);

#endif // CLIENT_H
