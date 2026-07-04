#ifndef SERVER_H
#define SERVER_H

#include "../core/config.h"

/**
 * ============================================================================
 * SERVER ENGINE
 * ============================================================================
 * This file boots up the main infinite server loop. It listens for incoming
 * connections and dispatches them to the `server_handlers.c` logic.
 */

/**
 * @brief Starts the infinite server loop.
 * Binds to the port specified in lantransfer.json and waits for connections.
 * 
 * @param config Pointer to the parsed configuration struct.
 * @return int 0 on graceful exit, -1 on fatal binding failure.
 */
int start_server(Config *config);

#endif // SERVER_H
