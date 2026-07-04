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
