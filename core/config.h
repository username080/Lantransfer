#ifndef CONFIG_H
#define CONFIG_H

/**
 * ============================================================================
 * CONFIGURATION ARCHITECTURE
 * ============================================================================
 * This module is responsible for reading the `lantransfer.json` file from disk
 * and parsing it into a C-struct that the rest of the application can use.
 */

#define MAX_CONFIG_STR_LEN 256

/**
 * @brief Defines whether this specific binary is acting as the central Server
 * or if it is being used as a Client tool.
 */
typedef enum {
    MODE_SERVER,
    MODE_CLIENT,
    MODE_UNKNOWN
} AppMode;

/**
 * @brief The master configuration structure. This holds everything needed
 * to start the networking engines.
 */
typedef struct {
    AppMode mode;
    int port;                                 // e.g. 7272
    char server_ip[MAX_CONFIG_STR_LEN];       // e.g. "192.168.1.16"
    char username[MAX_CONFIG_STR_LEN];        // Used for building cache paths
    char client_cache_dir[MAX_CONFIG_STR_LEN]; // Optional override path
    char server_cache_dir[MAX_CONFIG_STR_LEN]; // Optional override path
} Config;

/**
 * @brief Reads a JSON file from disk and parses it into the Config struct.
 * 
 * @param filename The path to the JSON file (usually "lantransfer.json").
 * @param config   A pointer to the Config struct to populate.
 * @return int     0 on success, -1 if the file doesn't exist or is corrupt.
 */
int load_config(const char *filename, Config *config);

#endif // CONFIG_H
