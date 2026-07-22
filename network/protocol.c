#include "protocol.h"
#include "socket.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

/**
 * Packs the action intent and the target path into a neat binary header and sends it.
 */
int protocol_send_header(int sockfd, uint8_t action, const char *target_path) {
    uint16_t len = strlen(target_path);
    uint16_t net_len = htons(len); // Network byte order (endianness safety)

    uint32_t buf_len = 1 + 2 + len;
    char *buf = malloc(buf_len);
    if (!buf) return -1;

    buf[0] = action;
    memcpy(buf + 1, &net_len, 2);
    if (len > 0) {
        memcpy(buf + 3, target_path, len);
    }

    int ret = send_all(sockfd, buf, buf_len);
    free(buf);
    return ret <= 0 ? -1 : 0;
}

/**
 * Sits and waits to receive the initial handshake.
 */
int protocol_recv_header(int sockfd, uint8_t *action, char *target_path, size_t max_len) {
    if (recv_all(sockfd, action, 1) <= 0) return -1;

    uint16_t net_len;
    if (recv_all(sockfd, &net_len, 2) <= 0) return -1;
    uint16_t len = ntohs(net_len);

    if (len >= max_len) {
        return -1; // Security: Prevent buffer overflow if client sends a malicious length
    }

    if (len > 0) {
        if (recv_all(sockfd, target_path, len) <= 0) return -1;
    }
    target_path[len] = '\0';
    return 0;
}

/**
 * Internally handles streaming a single specific file.
 * Format: [Type (0x00)] [RelPathLen] [RelPath] [FileSize (8 bytes)] [Binary Data]
 */
static int send_single_file(int sockfd, const char *local_path, const char *rel_path) {
    struct stat st;
    if (stat(local_path, &st) < 0) return -1;

    uint8_t type = ITEM_TYPE_FILE;
    uint16_t rel_len = strlen(rel_path);
    uint16_t net_rel_len = htons(rel_len);
    uint64_t file_size = st.st_size;
    
    // Split the 64-bit size into two 32-bit chunks to safely bypass endianness architectures
    uint32_t high = htonl(file_size >> 32);
    uint32_t low = htonl(file_size & 0xFFFFFFFF);

    // Blast the header info over the wire
    uint32_t header_len = 1 + 2 + rel_len + 8;
    char *header_buf = malloc(header_len);
    if (!header_buf) return -1;
    
    header_buf[0] = type;
    memcpy(header_buf + 1, &net_rel_len, 2);
    memcpy(header_buf + 3, rel_path, rel_len);
    memcpy(header_buf + 3 + rel_len, &high, 4);
    memcpy(header_buf + 7 + rel_len, &low, 4);
    
    if (send_all(sockfd, header_buf, header_len) <= 0) {
        free(header_buf);
        return -1;
    }
    free(header_buf);

    // Open the actual file to send its contents
    FILE *f = fopen(local_path, "rb");
    if (!f) {
        perror("fopen");
        return -1;
    }

    // A massive 256KB memory chunk for max throughput, allocated on heap to save stack space
    size_t chunk_size = 262144;
    char *buf = malloc(chunk_size);
    if (!buf) {
        fclose(f);
        return -1;
    }
    
    size_t n;
    uint64_t sent_bytes = 0;
    
    if (file_size == 0) {
        print_progress(0, 0, rel_path);
    } else {
        print_progress(0, file_size, rel_path);
        int last_percent = 0;
        // Loop until EOF
        while ((n = fread(buf, 1, chunk_size, f)) > 0) {
            if (send_all(sockfd, buf, n) < 0) {
                free(buf);
                fclose(f);
                return -1;
            }
            sent_bytes += n;
            int current_percent = (int)((float)sent_bytes / file_size * 100.0);
            if (current_percent > last_percent || sent_bytes == file_size) {
                print_progress(sent_bytes, file_size, rel_path);
                last_percent = current_percent;
            }
        }
    }
    
    free(buf);
    fclose(f);
    return 0;
}

/**
 * A recursive function that digs through a folder structure.
 * If it finds a file, it streams it. If it finds a folder, it calls itself!
 */
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
        // Skip current and parent directory pointers
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, sizeof(path), "%s/%s", base_local, entry->d_name);
        snprintf(rel, sizeof(rel), "%s/%s", base_rel, entry->d_name);

        if (is_directory(path)) {
            traverse_and_send(sockfd, path, rel); // Recursive dig
        } else {
            send_single_file(sockfd, path, rel);
        }
    }
    closedir(dir);
    return 0;
}

/**
 * The public function called by the rest of the app to trigger a massive transfer.
 */
int protocol_send_path(int sockfd, const char *local_path, const char *base_name) {
    int ret = traverse_and_send(sockfd, local_path, base_name);
    
    // Once everything is done, send the magic 0xFF byte to tell the receiver to close shop.
    uint8_t type = ITEM_TYPE_EOT;
    send_all(sockfd, &type, 1);
    return ret;
}

/**
 * The massive reception loop. It reads the binary stream forever until it hits 0xFF.
 */
int protocol_recv_stream(int sockfd, const char *destination_dir) {
    while (1) {
        // Read the very first byte to determine what we're looking at
        uint8_t type;
        if (recv_all(sockfd, &type, 1) <= 0) return -1;

        if (type == ITEM_TYPE_EOT) {
            break; // 0xFF received. The transfer is 100% complete!
        }

        if (type == ITEM_TYPE_FILE) {
            // Unpack the relative path string length
            uint16_t net_rel_len;
            if (recv_all(sockfd, &net_rel_len, 2) <= 0) return -1;
            uint16_t rel_len = ntohs(net_rel_len);

            // Read path string and the 8-byte file size all at once
            uint32_t rest_len = rel_len + 8;
            char *rest_buf = malloc(rest_len);
            if (!rest_buf) return -1;
            if (recv_all(sockfd, rest_buf, rest_len) <= 0) {
                free(rest_buf);
                return -1;
            }

            char rel_path[4096];
            memcpy(rel_path, rest_buf, rel_len);
            rel_path[rel_len] = '\0';

            // Unpack the 64-bit file size
            uint32_t high, low;
            memcpy(&high, rest_buf + rel_len, 4);
            memcpy(&low, rest_buf + rel_len + 4, 4);
            free(rest_buf);
            uint64_t file_size = ((uint64_t)ntohl(high) << 32) | ntohl(low);

            // Construct the final disk path
            char full_path[4096];
            join_path(full_path, sizeof(full_path), destination_dir, rel_path);

            // Magically create all parent directories on the disk just in time
            char *last_slash = strrchr(full_path, '/');
            if (last_slash) {
                *last_slash = '\0';
                mkdir_p(full_path);
                *last_slash = '/';
            }

            // Open the new file and start dumping binary data into it
            FILE *f = fopen(full_path, "wb");
            if (!f) {
                perror("fopen destination");
                return -1;
            }

            size_t chunk_size = 262144;
            char *buf = malloc(chunk_size);
            if (!buf) {
                fclose(f);
                return -1;
            }
            uint64_t received = 0;
            
            if (file_size == 0) {
                print_progress(0, 0, rel_path);
            } else {
                print_progress(0, file_size, rel_path);
                int last_percent = 0;
                // Download loop
                while (received < file_size) {
                    size_t to_read = chunk_size;
                    // Prevent reading past the end of THIS file into the NEXT file's header!
                    if (file_size - received < to_read) {
                        to_read = file_size - received;
                    }
                    
                    ssize_t n = recv_all(sockfd, buf, to_read);
                    if (n <= 0) {
                        free(buf);
                        fclose(f);
                        return -1;
                    }
                    fwrite(buf, 1, n, f);
                    received += n;
                    int current_percent = (int)((float)received / file_size * 100.0);
                    if (current_percent > last_percent || received == file_size) {
                        print_progress(received, file_size, rel_path);
                        last_percent = current_percent;
                    }
                }
            }
            
            free(buf);
            fclose(f);
        } else {
            fprintf(stderr, "Security Alert: Unknown item type: %x. Terminating connection.\n", type);
            return -1;
        }
    }
    return 0;
}
