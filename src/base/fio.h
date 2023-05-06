#pragma once

//Contains File Input/Output (hence fio)

#include "types.h"
#include "memory.h"

//TODO: Make this rely on the platform layer
#include <stdio.h>

//Utils

//Reads an entire file data and allocates it in memory, freed with oo_free(...)
void *fio_read_whole_file(const char* filepath, u64 *file_size);