#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <stddef.h>

/**
 * ============================================================================
 * SOCKET PRIMITIVES
 * ============================================================================
 * This file handles the absolute lowest-level TCP networking. It hides the 
 * ugly Linux socket C APIs (like AF_INET, setsockopt, sockaddr_in) so the rest 
 * of the application can just say "create_server" or "send_all".
 */

/**
 * @brief Boots up a TCP listening socket on the given port.
 * It automatically configures SO_REUSEADDR so that if the server crashes,
 * it can instantly restart without waiting for the OS to release the port.
 * 
 * @param port The port number to listen on (e.g. 7272).
 * @return int The socket file descriptor, or -1 if the port is blocked.
 */
int create_server_socket(int port);

/**
 * @brief Dials out to a remote server to establish a TCP connection.
 * 
 * @param ip   The IPv4 address of the server (e.g. "192.168.1.16").
 * @param port The port the server is listening on.
 * @return int The connected socket file descriptor, or -1 if connection failed.
 */
int create_client_socket(const char *ip, int port);

/**
 * @brief Guaranteed network transmission.
 * Standard `send()` is not guaranteed to send all bytes at once. It might send 
 * half a buffer and stop. `send_all` uses a mathematical while-loop to ensure 
 * 100% of the requested bytes are pushed over the wire.
 * 
 * @param sockfd The socket to push data into.
 * @param buf    The raw memory buffer to send.
 * @param len    The exact number of bytes to send.
 * @return ssize_t The total bytes sent, or -1 if the connection snapped.
 */
ssize_t send_all(int sockfd, const void *buf, size_t len);

/**
 * @brief Guaranteed network reception.
 * Standard `recv()` might read only 10 bytes even if you asked for 100.
 * `recv_all` loops and blocks the thread until every single requested byte 
 * is safely pulled from the network card into memory.
 * 
 * @param sockfd The socket to read from.
 * @param buf    The memory buffer to dump the data into.
 * @param len    The exact number of bytes to wait for.
 * @return ssize_t The total bytes received, or -1 if the connection dropped.
 */
ssize_t recv_all(int sockfd, void *buf, size_t len);

#endif // SOCKET_H
