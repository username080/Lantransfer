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

#endif // CLIENT_H
