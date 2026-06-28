#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#define ACTION_SEND 0x01
#define ACTION_GET  0x02

#define ITEM_TYPE_FILE 0x00
#define ITEM_TYPE_EOT  0xFF  // End of transaction

/**
 * Sends the initial session header.
 * action: ACTION_SEND or ACTION_GET
 * target_path: the file or directory being sent or requested.
 */
int protocol_send_header(int sockfd, uint8_t action, const char *target_path);

/**
 * Receives the initial session header.
 */
int protocol_recv_header(int sockfd, uint8_t *action, char *target_path, size_t max_len);

/**
 * Recursively sends a file or directory.
 * sockfd: connected socket
 * local_path: the actual path on disk to send (e.g., "/tmp/my_folder")
 * base_name: the logical base name to use in relative paths (e.g., "my_folder")
 * Returns 0 on success, -1 on failure.
 */
int protocol_send_path(int sockfd, const char *local_path, const char *base_name);

/**
 * Receives a stream of files and saves them into the destination directory.
 * destination_dir: The folder where items will be saved.
 * Returns 0 on success, -1 on failure.
 */
int protocol_recv_stream(int sockfd, const char *destination_dir);

/**
 * Sends a single error message back to close the connection gracefully (optional enhancement).
 */
int protocol_send_error(int sockfd, const char *error_msg);

#endif // PROTOCOL_H
