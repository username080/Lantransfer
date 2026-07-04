#include "protocol.h"
#include "socket.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

int protocol_send_header(int sockfd, uint8_t action, const char *target_path) {
    uint16_t len = strlen(target_path);
    uint16_t net_len = htons(len);

    if (send_all(sockfd, &action, 1) <= 0) return -1;
    if (send_all(sockfd, &net_len, 2) <= 0) return -1;
    if (len > 0) {
        if (send_all(sockfd, target_path, len) <= 0) return -1;
    }
    return 0;
}

int protocol_recv_header(int sockfd, uint8_t *action, char *target_path, size_t max_len) {
    if (recv_all(sockfd, action, 1) <= 0) return -1;

    uint16_t net_len;
    if (recv_all(sockfd, &net_len, 2) <= 0) return -1;
    uint16_t len = ntohs(net_len);

    if (len >= max_len) {
        return -1; // Path too long
    }

    if (len > 0) {
        if (recv_all(sockfd, target_path, len) <= 0) return -1;
    }
    target_path[len] = '\0';
    return 0;
}

static int send_single_file(int sockfd, const char *local_path, const char *rel_path) {
    struct stat st;
    if (stat(local_path, &st) < 0) return -1;

    uint8_t type = ITEM_TYPE_FILE;
    uint16_t rel_len = strlen(rel_path);
    uint16_t net_rel_len = htons(rel_len);
    uint64_t file_size = st.st_size;
    
    // For portability, handle endianness for 64-bit size (simplification: assume little/big endian mapping is not strictly required if same architecture, but let's do it safely)
    uint32_t high = htonl(file_size >> 32);
    uint32_t low = htonl(file_size & 0xFFFFFFFF);

    if (send_all(sockfd, &type, 1) <= 0) return -1;
    if (send_all(sockfd, &net_rel_len, 2) <= 0) return -1;
    if (send_all(sockfd, rel_path, rel_len) <= 0) return -1;
    if (send_all(sockfd, &high, 4) <= 0) return -1;
    if (send_all(sockfd, &low, 4) <= 0) return -1;

    FILE *f = fopen(local_path, "rb");
    if (!f) {
        perror("fopen");
        return -1;
    }

    char buf[8192];
    size_t n;
    uint64_t sent_bytes = 0;
    
    if (file_size == 0) {
        print_progress(0, 0, rel_path);
    } else {
        print_progress(0, file_size, rel_path);
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
            if (send_all(sockfd, buf, n) < 0) {
                fclose(f);
                return -1;
            }
            sent_bytes += n;
            print_progress(sent_bytes, file_size, rel_path);
        }
    }
    
    fclose(f);
    return 0;
}

static int traverse_and_send(int sockfd, const char *base_local, const char *base_rel) {
    if (!is_directory(base_local)) {
        return send_single_file(sockfd, base_local, base_rel);
    }

    DIR *dir = opendir(base_local);
    if (!dir) return -1;

    struct dirent *entry;
    char path[4096];
    char rel[4096];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, sizeof(path), "%s/%s", base_local, entry->d_name);
        snprintf(rel, sizeof(rel), "%s/%s", base_rel, entry->d_name);

        if (is_directory(path)) {
            traverse_and_send(sockfd, path, rel);
        } else {
            send_single_file(sockfd, path, rel);
        }
    }
    closedir(dir);
    return 0;
}

int protocol_send_path(int sockfd, const char *local_path, const char *base_name) {
    int ret = traverse_and_send(sockfd, local_path, base_name);
    
    // Send End of Transaction
    uint8_t type = ITEM_TYPE_EOT;
    send_all(sockfd, &type, 1);
    return ret;
}

int protocol_recv_stream(int sockfd, const char *destination_dir) {
    while (1) {
        uint8_t type;
        if (recv_all(sockfd, &type, 1) <= 0) return -1;

        if (type == ITEM_TYPE_EOT) {
            break;
        }

        if (type == ITEM_TYPE_FILE) {
            uint16_t net_rel_len;
            if (recv_all(sockfd, &net_rel_len, 2) <= 0) return -1;
            uint16_t rel_len = ntohs(net_rel_len);

            char rel_path[4096];
            if (recv_all(sockfd, rel_path, rel_len) <= 0) return -1;
            rel_path[rel_len] = '\0';

            uint32_t high, low;
            if (recv_all(sockfd, &high, 4) <= 0) return -1;
            if (recv_all(sockfd, &low, 4) <= 0) return -1;
            uint64_t file_size = ((uint64_t)ntohl(high) << 32) | ntohl(low);

            char full_path[4096];
            join_path(full_path, sizeof(full_path), destination_dir, rel_path);

            // Create directories if needed
            char *last_slash = strrchr(full_path, '/');
            if (last_slash) {
                *last_slash = '\0';
                mkdir_p(full_path);
                *last_slash = '/';
            }

            FILE *f = fopen(full_path, "wb");
            if (!f) {
                perror("fopen destination");
                return -1;
            }

            char buf[8192];
            uint64_t received = 0;
            
            if (file_size == 0) {
                print_progress(0, 0, rel_path);
            } else {
                print_progress(0, file_size, rel_path);
                while (received < file_size) {
                    size_t to_read = sizeof(buf);
                    if (file_size - received < to_read) {
                        to_read = file_size - received;
                    }
                    ssize_t n = recv_all(sockfd, buf, to_read);
                    if (n <= 0) {
                        fclose(f);
                        return -1;
                    }
                    fwrite(buf, 1, n, f);
                    received += n;
                    print_progress(received, file_size, rel_path);
                }
            }
            
            fclose(f);
        } else {
            fprintf(stderr, "Unknown item type: %x\n", type);
            return -1;
        }
    }
    return 0;
}
