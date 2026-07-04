#include "config.h"
#include "../utils/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Reads the `lantransfer.json` file entirely into memory, uses the third-party
 * cJSON library to parse it, and then safely extracts the values into our struct.
 */
int load_config(const char *filename, Config *config) {
    // 1. Open the file
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: Could not open config file '%s'\n", filename);
        return -1;
    }

    // 2. Measure the exact size of the file
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET); // Rewind back to the start

    // 3. Allocate memory and read the entire file into a string
    char *json_data = malloc(fsize + 1);
    if (!json_data) {
        fclose(f);
        return -1; // Out of memory!
    }
    fread(json_data, 1, fsize, f);
    json_data[fsize] = '\0'; // Null-terminate it so C knows it's a string
    fclose(f);

    // 4. Hand the string over to the JSON parser
    cJSON *root = cJSON_Parse(json_data);
    free(json_data); // Free the raw string now that it's parsed
    if (!root) {
        fprintf(stderr, "Error: Failed to parse JSON config\n");
        return -1;
    }

    // 5. Establish safe default values
    config->mode = MODE_UNKNOWN;
    config->port = 8080;
    strcpy(config->server_ip, "127.0.0.1");
    strcpy(config->username, "your_username");
    config->client_cache_dir[0] = '\0';
    config->server_cache_dir[0] = '\0';

    // 6. Extract 'mode' (server vs client)
    cJSON *mode_item = cJSON_GetObjectItem(root, "mode");
    if (cJSON_IsString(mode_item) && (mode_item->valuestring != NULL)) {
        if (strcmp(mode_item->valuestring, "server") == 0) {
            config->mode = MODE_SERVER;
        } else if (strcmp(mode_item->valuestring, "client") == 0) {
            config->mode = MODE_CLIENT;
        }
    }

    // 7. Extract 'port'
    cJSON *port_item = cJSON_GetObjectItem(root, "port");
    if (cJSON_IsNumber(port_item)) {
        config->port = port_item->valueint;
    }

    // 8. Extract 'server_ip'
    cJSON *ip_item = cJSON_GetObjectItem(root, "server_ip");
    if (cJSON_IsString(ip_item) && (ip_item->valuestring != NULL)) {
        strncpy(config->server_ip, ip_item->valuestring, MAX_CONFIG_STR_LEN - 1);
        config->server_ip[MAX_CONFIG_STR_LEN - 1] = '\0';
    }

    // 9. Extract 'username'
    cJSON *username_item = cJSON_GetObjectItem(root, "username");
    if (cJSON_IsString(username_item) && (username_item->valuestring != NULL)) {
        strncpy(config->username, username_item->valuestring, MAX_CONFIG_STR_LEN - 1);
        config->username[MAX_CONFIG_STR_LEN - 1] = '\0';
    }

    // 10. Extract 'client_cache_dir'
    cJSON *client_cache_item = cJSON_GetObjectItem(root, "client_cache_dir");
    if (cJSON_IsString(client_cache_item) && (client_cache_item->valuestring != NULL)) {
        strncpy(config->client_cache_dir, client_cache_item->valuestring, MAX_CONFIG_STR_LEN - 1);
        config->client_cache_dir[MAX_CONFIG_STR_LEN - 1] = '\0';
    }

    // 11. Extract 'server_cache_dir'
    cJSON *server_cache_item = cJSON_GetObjectItem(root, "server_cache_dir");
    if (cJSON_IsString(server_cache_item) && (server_cache_item->valuestring != NULL)) {
        strncpy(config->server_cache_dir, server_cache_item->valuestring, MAX_CONFIG_STR_LEN - 1);
        config->server_cache_dir[MAX_CONFIG_STR_LEN - 1] = '\0';
    }

    cJSON_Delete(root); // Cleanup JSON memory
    return 0;
}
