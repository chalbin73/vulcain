#pragma once

#include "types.h"

#include <memory.h>
#include <stdlib.h>
#include <string.h>
// NOTE: This translation unit contains the necessary abstractions of the platform

// Memory:
void *platform_alloc(u64 size);
void *platform_realloc(void *ptr, u64 size);
void  platform_free(void *ptr);
void  platform_memset(void *ptr, u8 data, u64 byte_count);
void  platform_memcpy(void *dest, void *src, u64 byte_count);