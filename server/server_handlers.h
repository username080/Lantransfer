#ifndef SERVER_HANDLERS_H
#define SERVER_HANDLERS_H

#include "../core/config.h"

/**
 * @brief Handles the ACTION_SEND command.
 */
void handle_client_send(int client_fd, const char *target_path, const char *base_cache);

/**
 * @brief Handles the ACTION_GET command.
 */
void handle_client_get(int client_fd, const char *target_path);

/**
 * @brief Handles the ACTION_GET_MOVE command (sends file, then deletes server copy).
 */
void handle_client_get_move(int client_fd, const char *target_path);

/**
 * @brief Handles the ACTION_EXEC and ACTION_EXEC_DETACH commands.
 */
void handle_client_exec(int client_fd, const char *target_path, int detach, const char *base_cache);

/**
 * @brief Handles the ACTION_LIST_TASKS command.
 */
void handle_client_list_tasks(int client_fd, const char *base_cache);

/**
 * @brief Handles the ACTION_ATTACH command.
 */
void handle_client_attach(int client_fd, const char *task_id, const char *base_cache);

#endif // SERVER_HANDLERS_H
