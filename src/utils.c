#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

void get_timestamp_str(char *buf, size_t size) {
    time_t t = time(NULL);
    struct tm tm_info;
    localtime_r(&t, &tm_info);
    strftime(buf, size, "%Y-%m-%d_%H-%M-%S", &tm_info);
}

int mkdir_p(const char *path) {
    char tmp[4096];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if(tmp[len - 1] == '/')
        tmp[len - 1] = '\0';
        
    for(p = tmp + 1; *p; p++) {
        if(*p == '/') {
            *p = '\0';
            if(mkdir(tmp, 0755) < 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
        }
    }
    if(mkdir(tmp, 0755) < 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

int is_directory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
}

void join_path(char *dest, size_t size, const char *dir, const char *file) {
    size_t len1 = strlen(dir);
    if (len1 > 0 && dir[len1 - 1] == '/') {
        snprintf(dest, size, "%s%s", dir, file);
    } else {
        snprintf(dest, size, "%s/%s", dir, file);
    }
}

#include <limits.h>

void make_absolute_path(char *dest, size_t size, const char *path) {
    // If no path is provided, default to current working directory
    if (path == NULL || strlen(path) == 0) {
        if (getcwd(dest, size) == NULL) {
            snprintf(dest, size, ".");
        }
        return;
    }

    // realpath() securely resolves symlinks, /./, and /../, returning a canonical absolute path.
    // It requires the target to actually exist on the filesystem to work.
    char resolved_path[PATH_MAX];
    if (realpath(path, resolved_path) != NULL) {
        // Safely format the resolved path into the destination buffer
        snprintf(dest, size, "%s", resolved_path);
    } else {
        // Fallback: If the path doesn't exist yet (e.g., when specifying a new file),
        // realpath() returns NULL. In this case, we manually combine CWD and the path.
        if (path[0] == '/') {
            snprintf(dest, size, "%s", path);
        } else {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                snprintf(dest, size, "%s/%s", cwd, path);
            } else {
                snprintf(dest, size, "%s", path);
            }
        }
    }
}

void print_progress(uint64_t current, uint64_t total, const char *prefix) {
    if (total == 0) {
        printf("\r%s [==================================================] 100%%\n", prefix);
        fflush(stdout);
        return;
    }
    
    int width = 50;
    float ratio = (float)current / (float)total;
    int pos = width * ratio;
    
    printf("\r%s [", prefix);
    for (int i = 0; i < width; ++i) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %3d%%", (int)(ratio * 100.0));
    
    if (current == total) {
        printf("\n");
    }
    fflush(stdout);
}
