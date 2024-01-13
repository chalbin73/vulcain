#pragma once

#include "types.h"

// NOTE: This translation unit contains the necessary abstractions of the platform

// Memory:
void   *platform_alloc(u64    size);
void   *platform_realloc(void *ptr, u64 size);
void    platform_free(void   *ptr);
void    platform_memset(void *ptr, u8 data, u64 byte_count);
void    platform_memcpy(void *dest, void *src, u64 byte_count);
i32     platform_memcmp(void *a, void *b, u64 n);
void   *platform_memmove(void *a, void *b, u64 n);

// Returns a millisecond time, such that if it is called n milliseconds appart, the difference between the two numbers should be n
u64     platform_millis(void);

