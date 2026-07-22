#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

/**
 * Creates the master server socket.
 */
int create_server_socket(int port) {
    // AF_INET = IPv4, SOCK_STREAM = TCP Connection
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    // Force the OS to let us bind to this port immediately, even if it's in a TIME_WAIT state.
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        close(sockfd);
        return -1;
    }

    int tcp_opt = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &tcp_opt, sizeof(tcp_opt));

    int sock_buf = 8388608;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sock_buf, sizeof(sock_buf));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &sock_buf, sizeof(sock_buf));

    // Configure the address struct to listen on ALL network interfaces (0.0.0.0)
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port); // Convert port to Network Byte Order (Big Endian)

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        return -1;
    }

    // Tell the OS to start queuing incoming connections (up to 5 at a time)
    if (listen(sockfd, 5) < 0) {
        perror("listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/**
 * Dials out and connects to a server.
 */
int create_client_socket(const char *ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    int tcp_opt = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &tcp_opt, sizeof(tcp_opt));

    int sock_buf = 8388608;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sock_buf, sizeof(sock_buf));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &sock_buf, sizeof(sock_buf));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    // Convert string IP ("192.168.1.1") into binary network format
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }

    // Attempt the actual TCP Handshake
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/**
 * Loops the send() syscall until all bytes are transmitted.
 */
ssize_t send_all(int sockfd, const void *buf, size_t len) {
    size_t total_sent = 0;
    const char *p = (const char *)buf;

    while (total_sent < len) {
        // Send whatever is left: len - total_sent
        // MSG_NOSIGNAL prevents the application from crashing if the connection drops!
        ssize_t sent = send(sockfd, p + total_sent, len - total_sent, MSG_NOSIGNAL);
        
        if (sent < 0) {
            if (errno == EINTR) continue; // Interrupted by a system signal, just try again
            perror("send");
            return -1;
        }
        
        if (sent == 0) {
            break; // Connection closed gracefully by the other side
        }
        
        total_sent += sent; // Move our pointer forward
    }

    return total_sent;
}

/**
 * Loops the recv() syscall until all requested bytes arrive.
 */
ssize_t recv_all(int sockfd, void *buf, size_t len) {
    size_t total_recv = 0;
    char *p = (char *)buf;

    while (total_recv < len) {
        // MSG_WAITALL forces the kernel to wait until all requested bytes arrive,
        // reducing CPU context switches by up to 180x for large transfers!
        ssize_t received = recv(sockfd, p + total_recv, len - total_recv, MSG_WAITALL);
        
        if (received < 0) {
            if (errno == EINTR) continue;
            perror("recv");
            return -1;
        }
        
        if (received == 0) {
            // The peer hung up before sending all the bytes we asked for.
            return total_recv;
        }
        
        total_recv += received;
    }

    return total_recv;
}
