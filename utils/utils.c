#define _POSIX_C_SOURCE 200809L
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>

/**
 * Grabs the current system time and formats it beautifully.
 */
void get_timestamp_str(char *buf, size_t size) {
    time_t t = time(NULL);
    struct tm tm_info;
    localtime_r(&t, &tm_info);
    
    // Format: Year-Month-Day_Hour-Minute-Second
    strftime(buf, size, "%Y-%m-%d_%H-%M-%S", &tm_info);
}

/**
 * Loops through the requested directory string and recursively
 * creates every parent folder leading up to the final destination.
 */
int mkdir_p(const char *path) {
    char tmp[4096];
    char *p = NULL;
    size_t len;

    // Make a local copy of the path so we can safely manipulate it
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    
    // Strip any trailing slash purely for clean parsing
    if(len > 0 && tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }
        
    // Walk down the path character by character
    for(p = tmp + 1; *p; p++) {
        // Every time we hit a slash, we have found a folder boundary
        if(*p == '/') {
            *p = '\0'; // Temporarily terminate the string right here
            
            // Try to create this specific parent folder
            if(mkdir(tmp, 0755) < 0 && errno != EEXIST) {
                return -1; // Fail if it's not simply "already exists"
            }
            
            *p = '/'; // Put the slash back and continue down the chain!
        }
    }
    
    // Create the final target directory at the very end
    if(mkdir(tmp, 0755) < 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

/**
 * Standard system stat check to see if a path is actually a folder.
 */
int is_directory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
}

/**
 * Safely concats two paths so we don't accidentally end up with 
 * ugly paths like "/home/sysop//cache"
 */
void join_path(char *dest, size_t size, const char *dir, const char *file) {
    size_t len1 = strlen(dir);
    if (len1 > 0 && dir[len1 - 1] == '/') {
        snprintf(dest, size, "%s%s", dir, file);
    } else {
        snprintf(dest, size, "%s/%s", dir, file);
    }
}

/**
 * A highly secure absolute path resolver. This prevents path traversal 
 * attacks (like passing "../../etc/passwd") by forcing the OS to map
 * the exact, canonical disk path using `realpath()`.
 */
void make_absolute_path(char *dest, size_t size, const char *path) {
    // If the user didn't provide a path at all, just default to where we are right now.
    if (path == NULL || strlen(path) == 0) {
        if (getcwd(dest, size) == NULL) {
            snprintf(dest, size, ".");
        }
        return;
    }

    char resolved_path[PATH_MAX];
    
    // Attempt to cryptographically resolve the path on the disk
    if (realpath(path, resolved_path) != NULL) {
        snprintf(dest, size, "%s", resolved_path);
    } else {
        // If the path DOES NOT exist yet (like trying to save a brand new file),
        // realpath() will fail. In that case, we fall back to manually creating
        // the absolute path by slapping the current working directory in front of it.
        if (path[0] == '/') {
            snprintf(dest, size, "%s", path); // It's already absolute
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

/**
 * Mathematical progress bar drawer. Calculates the percentage float
 * and maps it onto a 50-character visual grid.
 */
void print_progress(uint64_t current, uint64_t total, const char *prefix) {
    if (total == 0) {
        printf("\r%s [==================================================] 100%%\n", prefix);
        fflush(stdout);
        return;
    }
    
    int width = 50;
    float ratio = (float)current / (float)total;
    int pos = (int)(width * ratio);
    
    printf("\r%s [", prefix);
    for (int i = 0; i < width; ++i) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">"); // The leading edge arrow
        else printf(" ");
    }
    printf("] %3d%%", (int)(ratio * 100.0));
    
    // Once we hit 100%, break the terminal line so next text is clean.
    if (current == total) {
        printf("\n");
    }
    fflush(stdout);
}

/**
 * Resolves the master cache directory path based on the JSON configuration,
 * and forces the operating system to create it if it doesn't exist.
 */
void ensure_server_cache_dir(Config *config, char *base_cache, size_t size) {
    if (strlen(config->server_cache_dir) > 0) {
        make_absolute_path(base_cache, size, config->server_cache_dir);
    } else {
        snprintf(base_cache, size, "/home/%s/lantransfercache", config->username);
    }
    mkdir_p(base_cache);
    
    char live_logs[4096], archived_logs[4096];
    snprintf(live_logs, sizeof(live_logs), "%s/live_logs", base_cache);
    snprintf(archived_logs, sizeof(archived_logs), "%s/archived_logs", base_cache);
    mkdir_p(live_logs);
    mkdir_p(archived_logs);
}

#include <sys/file.h>
#include <dirent.h>

unsigned long long get_process_starttime(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    
    char buf[1024];
    if (fgets(buf, sizeof(buf), f)) {
        char *rparen = strrchr(buf, ')');
        if (rparen) {
            unsigned long long starttime = 0;
            char *p = rparen + 2; 
            for (int i = 3; i < 22; i++) {
                p = strchr(p, ' ');
                if (!p) break;
                p++;
            }
            if (p) {
                sscanf(p, "%llu", &starttime);
                fclose(f);
                return starttime;
            }
        }
    }
    fclose(f);
    return 0;
}

void add_running_task(const char *base_cache, const char *task_id, pid_t pid, const char *command) {
    char file_path[4096];
    snprintf(file_path, sizeof(file_path), "%s/running_tasks.txt", base_cache);
    FILE *f = fopen(file_path, "a");
    if (!f) return;
    
    unsigned long long starttime = get_process_starttime(pid);
    
    int fd = fileno(f);
    flock(fd, LOCK_EX);
    fprintf(f, "%s %d %llu: %s\n", task_id, (int)pid, starttime, command);
    fflush(f);
    flock(fd, LOCK_UN);
    fclose(f);
}

void archive_log(const char *base_cache, const char *task_id) {
    char src[8192], dest[8192];
    snprintf(src, sizeof(src), "%s/live_logs/exec_output_%s.log", base_cache, task_id);
    snprintf(dest, sizeof(dest), "%s/archived_logs/exec_output_%s.log", base_cache, task_id);
    rename(src, dest);
}

void clean_dead_tasks(const char *base_cache) {
    char file_path[4096];
    snprintf(file_path, sizeof(file_path), "%s/running_tasks.txt", base_cache);
    FILE *f = fopen(file_path, "r+");
    if (!f) return;
    int fd = fileno(f);
    flock(fd, LOCK_EX);
    char *buffer = malloc(1024 * 1024);
    if (!buffer) { flock(fd, LOCK_UN); fclose(f); return; }
    buffer[0] = '\0';
    char line[4096];
    while(fgets(line, sizeof(line), f)) {
        char t_id[128];
        int pid;
        unsigned long long recorded_starttime;
        
        if (sscanf(line, "%127s %d %llu:", t_id, &pid, &recorded_starttime) == 3) {
            unsigned long long current_starttime = get_process_starttime(pid);
            if (current_starttime == 0 || current_starttime != recorded_starttime) {
                archive_log(base_cache, t_id);
                continue;
            }
        } else if (sscanf(line, "%127s %d:", t_id, &pid) == 2) {
            if (kill(pid, 0) == -1 && errno == ESRCH) {
                archive_log(base_cache, t_id);
                continue;
            }
        }
        strcat(buffer, line);
    }
    rewind(f);
    if (ftruncate(fd, 0) == 0) fputs(buffer, f);
    fflush(f);
    free(buffer);
    flock(fd, LOCK_UN);
    fclose(f);
}

void remove_running_task(const char *base_cache, const char *task_id) {
    char file_path[4096];
    snprintf(file_path, sizeof(file_path), "%s/running_tasks.txt", base_cache);
    
    FILE *f = fopen(file_path, "r+");
    if (!f) return;
    
    int fd = fileno(f);
    flock(fd, LOCK_EX);
    
    char *buffer = malloc(1024 * 1024); // 1MB
    if (!buffer) {
        flock(fd, LOCK_UN);
        fclose(f);
        return;
    }
    buffer[0] = '\0';
    
    char line[4096];
    int found = 0;
    while(fgets(line, sizeof(line), f)) {
        if (strncmp(line, task_id, strlen(task_id)) != 0) {
            strcat(buffer, line);
        } else {
            found = 1;
        }
    }
    
    rewind(f);
    if (ftruncate(fd, 0) == 0) {
        fputs(buffer, f);
    }
    fflush(f);
    
    free(buffer);
    flock(fd, LOCK_UN);
    fclose(f);
    
    if (found) {
        archive_log(base_cache, task_id);
    }
}

int remove_path(const char *path) {
    if (is_directory(path)) {
        DIR *dir = opendir(path);
        if (!dir) return -1;
        struct dirent *entry;
        char buf[4096];
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(buf, sizeof(buf), "%s/%s", path, entry->d_name);
            remove_path(buf);
        }
        closedir(dir);
        return rmdir(path);
    } else {
        return unlink(path);
    }
}
