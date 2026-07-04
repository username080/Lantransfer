#ifndef SERVER_HANDLERS_H
#define SERVER_HANDLERS_H

#include "../core/config.h"

/**
 * ============================================================================
 * SERVER BUSINESS LOGIC HANDLERS
 * ============================================================================
 * This file separates the bulky business logic out of the main server loop.
 * It contains the specific logic for exactly what to do when a client asks
 * to SEND, GET, or EXECUTE something.
 */

/**
 * @brief Handles the ACTION_SEND command.
 * Reads a binary stream from the client and saves the files onto the server's disk.
 */
void handle_client_send(int client_fd, const char *target_path, const char *base_cache);

/**
 * @brief Handles the ACTION_GET command.
 * Reads files from the server's disk and pushes them over the network to the client.
 */
void handle_client_get(int client_fd, const char *target_path);

/**
 * @brief Handles the ACTION_EXEC and ACTION_EXEC_SAVE commands.
 * Runs a remote script securely on the server, and optionally streams/saves the output.
 */
void handle_client_exec(int client_fd, const char *target_path, int save_to_server, const char *base_cache);

#endif // SERVER_HANDLERS_H
