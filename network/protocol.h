#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

/**
 * ============================================================================
 * LANTRANSFER BINARY PROTOCOL
 * ============================================================================
 * This file defines the exact byte-level rules of how Lantransfer talks.
 * Instead of using slow JSON or HTTP, we stream raw binary bytes to achieve
 * maximum network throughput.
 */

// --- Session Action Bytes ---
// These are the very first byte sent over a new socket connection.
// It instantly tells the server exactly what the client wants to do.
#define ACTION_SEND        0x01 // Client is pushing files to server
#define ACTION_GET         0x02 // Client is downloading files from server
#define ACTION_EXEC        0x03 // Client wants server to run a script
#define ACTION_EXEC_DETACH 0x05 // Client wants server to run a script in background
#define ACTION_LIST_TASKS  0x06 // Client asks for list of background tasks
#define ACTION_ATTACH      0x07 // Client wants to stream a background task's output
#define ACTION_GET_MOVE    0x08 // Client wants server to stream and delete file
#define ACTION_READ_LOG    0x09 // Client wants to list or stream archived logs

// --- Stream Item Types ---
// When streaming files, we need to know if the next bytes are a file or
// if we have reached the end of the transaction.
#define ITEM_TYPE_FILE   0x00 // The following bytes are a file header + data
#define ITEM_TYPE_EOT    0xFF // End of Transaction (Stop listening and close socket)

/**
 * @brief Sends the initial connection handshake.
 * Format: [1-byte Action] [2-byte Path Length] [N-byte Target Path]
 */
int protocol_send_header(int sockfd, uint8_t action, const char *target_path);

/**
 * @brief Receives the initial connection handshake.
 */
int protocol_recv_header(int sockfd, uint8_t *action, char *target_path, size_t max_len);

/**
 * @brief Walks through a directory, reads files, and streams them over the socket.
 * It sends [Type][PathLength][Path][Size][FileBytes] for every single file.
 */
int protocol_send_path(int sockfd, const char *local_path, const char *base_name);

/**
 * @brief Sits in a loop reading binary streams off the socket and saving them to disk.
 * It dynamically creates folders and files exactly as they arrive over the wire.
 */
int protocol_recv_stream(int sockfd, const char *destination_dir);

#endif // PROTOCOL_H
