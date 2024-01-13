#pragma once

// Contains File Input/Output (hence fio)

#include "memory.h"
#include "types.h"

// TODO: Make this rely on the platform layer
#include <stdio.h>

// Utils

/**
 * @brief Reads an entire file
 *
 * @param filepath The path of the file
 * @param file_size Out: the size of the file
 * @return void* A pointer to an allocated block of memory containing the file's contents, must be freed afterwise
 */
void   *fio_read_whole_file(const char *filepath, u64 *file_size);

