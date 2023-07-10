#include "../platform.h"
#include "../system.h"
#if SYSTEM_GNULINUX
// TODO: Use custom allocator
void *platform_alloc(u64 size)
{
    return malloc(size);
}

void *platform_realloc(void *ptr, u64 size)
{
    return realloc(ptr, size);
}

void platform_free(void *ptr)
{
    free(ptr);
}

void platform_memset(void *ptr, u8 data, u64 byte_count)
{
    memset(ptr, data, byte_count);
}

void platform_memcpy(void *dest, void *src, u64 byte_count)
{
    memcpy(dest, src, byte_count);
}

i32 platform_memcmp(void *a, void *b, u64 n)
{
    return memcmp(a, b, n);
}

void *platform_memmove(void *a, void *b, u64 n)
{
    return memmove(a, b, n);
}

#endif