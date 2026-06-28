#ifndef SOCKET_H
#define SOCKET_H

#include <stddef.h>
#include <sys/types.h>

/**
 * Creates a TCP server socket, binds to the given port, and starts listening.
 * Returns the socket file descriptor on success, or -1 on failure.
 */
int create_server_socket(int port);

/**
 * Creates a TCP client socket and connects to the server at the given IP and port.
 * Returns the socket file descriptor on success, or -1 on failure.
 */
int create_client_socket(const char *ip, int port);

/**
 * Reliably sends exactly 'len' bytes from 'buf' over the socket.
 * Returns the number of bytes sent (should be 'len') or -1 on error.
 */
ssize_t send_all(int sockfd, const void *buf, size_t len);

/**
 * Reliably receives exactly 'len' bytes into 'buf' from the socket.
 * Returns the number of bytes received (should be 'len') or <=0 on error/disconnect.
 */
ssize_t recv_all(int sockfd, void *buf, size_t len);

#endif // SOCKET_H
