#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

/**
 * Generates a timestamp string in the format YYYY-MM-DD_HH-MM-SS.
 * 'buf' should be at least 20 bytes long.
 */
void get_timestamp_str(char *buf, size_t size);

/**
 * Recursively creates directories for the given path.
 * Returns 0 on success, -1 on failure.
 */
int mkdir_p(const char *path);

/**
 * Check if path is a directory.
 * Returns 1 if directory, 0 if not (or on error).
 */
int is_directory(const char *path);

/**
 * Combine dir and file to a single path.
 */
void join_path(char *dest, size_t size, const char *dir, const char *file);

/**
 * Resolves to an absolute path.
 * If path is relative, it prepends the current working directory.
 */
void make_absolute_path(char *dest, size_t size, const char *path);

/**
 * Prints a progress bar to the terminal.
 */
void print_progress(uint64_t current, uint64_t total, const char *prefix);

#endif // UTILS_H
