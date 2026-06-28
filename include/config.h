#ifndef CONFIG_H
#define CONFIG_H

#define MAX_CONFIG_STR_LEN 256

typedef enum {
    MODE_SERVER,
    MODE_CLIENT,
    MODE_UNKNOWN
} AppMode;

typedef struct {
    AppMode mode;
    int port;
    char server_ip[MAX_CONFIG_STR_LEN];
    char username[MAX_CONFIG_STR_LEN];
    char client_cache_dir[MAX_CONFIG_STR_LEN];
    char server_cache_dir[MAX_CONFIG_STR_LEN];
} Config;

/**
 * Load configuration from a JSON file.
 * Returns 0 on success, -1 on failure.
 */
int load_config(const char *filename, Config *config);

#endif // CONFIG_H
