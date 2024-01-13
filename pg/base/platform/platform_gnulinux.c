#include "../platform.h"
#if SYSTEM_GNULINUX
#include "../system.h"
#include <time.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
// TODO: Use custom allocator
void   *platform_alloc(u64    size)
{
    return malloc(size);
}

void   *platform_realloc(void *ptr, u64 size)
{
    return realloc(ptr, size);
}

void    platform_free(void   *ptr)
{
    free(ptr);
}

void    platform_memset(void *ptr, u8 data, u64 byte_count)
{
    memset(ptr, data, byte_count);
}

void    platform_memcpy(void *dest, void *src, u64 byte_count)
{
    memcpy(dest, src, byte_count);
}

i32     platform_memcmp(void *a, void *b, u64 n)
{
    return memcmp(a, b, n);
}

void   *platform_memmove(void *a, void *b, u64 n)
{
    return memmove(a, b, n);
}

u64     platform_millis(void)
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);

    u64 millis = (time.tv_sec * 1e3) + (time.tv_nsec / 1e6);
    return millis;
}

#endif

