#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

/**
 * ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 * This file contains generic helper functions used throughout the application.
 * By keeping these functions isolated here, we ensure our main network and 
 * business logic files remain clean and abstract.
 */

/**
 * @brief Generates a highly readable timestamp string.
 * Used primarily for naming the log files and cache directories cleanly.
 * Example Output: "2026-07-04_18-48-46"
 * 
 * @param buf   The character buffer to store the resulting string.
 * @param size  The maximum size of the buffer (should be at least 20 bytes).
 */
void get_timestamp_str(char *buf, size_t size);

/**
 * @brief Recursively creates a deep directory structure (like `mkdir -p`).
 * If you ask it to create `/home/user/cache/data`, it will safely ensure
 * that every parent folder exists before creating the final one.
 * 
 * @param path  The absolute or relative directory path to create.
 * @return int  0 on complete success, or -1 if the operating system denied it.
 */
int mkdir_p(const char *path);

/**
 * @brief Checks if a specific path exists and is a directory (folder).
 * 
 * @param path  The path to check.
 * @return int  1 if it's a valid directory, 0 if it's a file or doesn't exist.
 */
int is_directory(const char *path);

/**
 * @brief Safely combines a folder path and a file name into one clean path.
 * It automatically handles missing or duplicate slashes (e.g. "dir" + "file" 
 * becomes "dir/file", and "dir/" + "file" remains "dir/file").
 * 
 * @param dest  The buffer where the final string will be placed.
 * @param size  The maximum size of the destination buffer.
 * @param dir   The parent directory path.
 * @param file  The file name to append.
 */
void join_path(char *dest, size_t size, const char *dir, const char *file);

/**
 * @brief Converts any relative path into a fully qualified, secure absolute path.
 * This is a critical security function! It uses the operating system's `realpath()` 
 * to resolve things like `/home/sysop/../sysop/file.txt` into `/home/sysop/file.txt`.
 * 
 * @param dest  The buffer to store the final absolute path.
 * @param size  The size of the destination buffer.
 * @param path  The raw, potentially relative user path.
 */
void make_absolute_path(char *dest, size_t size, const char *path);

/**
 * @brief Draws a beautiful, dynamic progress bar in the terminal.
 * Used during heavy file transfers so the user isn't left staring at a blank screen.
 * 
 * @param current  The amount of bytes transferred so far.
 * @param total    The total expected file size in bytes.
 * @param prefix   A string to display before the bar (e.g., "Downloading").
 */
void print_progress(uint64_t current, uint64_t total, const char *prefix);

#include "../core/config.h"

/**
 * @brief Resolves and forcefully creates the master server cache directory.
 * Centralizing this prevents ugly duplicate code across all the different routes.
 * 
 * @param config      The parsed configuration.
 * @param base_cache  The buffer to store the final resolved path.
 * @param size        The max size of the base_cache buffer.
 */
void ensure_server_cache_dir(Config *config, char *base_cache, size_t size);

/**
 * @brief Safely adds a running task to the tracking file using file locks.
 */
void add_running_task(const char *base_cache, const char *task_id, const char *command);

/**
 * @brief Safely removes a task from the tracking file using file locks.
 */
void remove_running_task(const char *base_cache, const char *task_id);

/**
 * @brief Recursively deletes a file or directory.
 */
int remove_path(const char *path);

#endif // UTILS_H
