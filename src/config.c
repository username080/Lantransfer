#include "config.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int load_config(const char *filename, Config *config) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: Could not open config file '%s'\n", filename);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *json_data = malloc(fsize + 1);
    if (!json_data) {
        fclose(f);
        return -1;
    }
    fread(json_data, 1, fsize, f);
    json_data[fsize] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(json_data);
    free(json_data);
    if (!root) {
        fprintf(stderr, "Error: Failed to parse JSON config\n");
        return -1;
    }

    // Default values
    config->mode = MODE_UNKNOWN;
    config->port = 8080;
    strcpy(config->server_ip, "127.0.0.1");
    strcpy(config->username, "your_username");
    config->client_cache_dir[0] = '\0';
    config->server_cache_dir[0] = '\0';

    cJSON *mode_item = cJSON_GetObjectItem(root, "mode");
    if (cJSON_IsString(mode_item) && (mode_item->valuestring != NULL)) {
        if (strcmp(mode_item->valuestring, "server") == 0) {
            config->mode = MODE_SERVER;
        } else if (strcmp(mode_item->valuestring, "client") == 0) {
            config->mode = MODE_CLIENT;
        }
    }

    cJSON *port_item = cJSON_GetObjectItem(root, "port");
    if (cJSON_IsNumber(port_item)) {
        config->port = port_item->valueint;
    }

    cJSON *ip_item = cJSON_GetObjectItem(root, "server_ip");
    if (cJSON_IsString(ip_item) && (ip_item->valuestring != NULL)) {
        strncpy(config->server_ip, ip_item->valuestring, MAX_CONFIG_STR_LEN - 1);
        config->server_ip[MAX_CONFIG_STR_LEN - 1] = '\0';
    }

    cJSON *username_item = cJSON_GetObjectItem(root, "username");
    if (cJSON_IsString(username_item) && (username_item->valuestring != NULL)) {
        strncpy(config->username, username_item->valuestring, MAX_CONFIG_STR_LEN - 1);
        config->username[MAX_CONFIG_STR_LEN - 1] = '\0';
    }

    cJSON *client_cache_item = cJSON_GetObjectItem(root, "client_cache_dir");
    if (cJSON_IsString(client_cache_item) && (client_cache_item->valuestring != NULL)) {
        strncpy(config->client_cache_dir, client_cache_item->valuestring, MAX_CONFIG_STR_LEN - 1);
        config->client_cache_dir[MAX_CONFIG_STR_LEN - 1] = '\0';
    }

    cJSON *server_cache_item = cJSON_GetObjectItem(root, "server_cache_dir");
    if (cJSON_IsString(server_cache_item) && (server_cache_item->valuestring != NULL)) {
        strncpy(config->server_cache_dir, server_cache_item->valuestring, MAX_CONFIG_STR_LEN - 1);
        config->server_cache_dir[MAX_CONFIG_STR_LEN - 1] = '\0';
    }

    cJSON_Delete(root);
    return 0;
}
