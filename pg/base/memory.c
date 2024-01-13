#include "memory.h"

void    memory_start()
{
    TRACE("Starting memory subsystem.");
    current_memory_usage = (struct mem_usage)
    {
        0
    };
}

void    memory_shutdown()
{
    for (int i = 0; i < MEMORY_TAG_COUNT; i++)
    {
        if (current_memory_usage.tags_memory_usage[i] != 0)
        {
            char buf[64];

            mem_size_get_pretty_string(current_memory_usage.tags_memory_usage[i], buf);
            WARN("Memory leak detected : %s : %s not freed.", memory_alloc_tag_names[i], buf);
        }
    }

    if (current_memory_usage.total_memory_usage != 0)
    {
        char buf[64];
        mem_size_get_pretty_string(current_memory_usage.total_memory_usage, buf);
        WARN("Total memory leak    : %s : %s not freed.", MEMORY_ALLOC_TOTAL_NAME, buf);
    }

    TRACE("Shutting memory subsystem down.");
    current_memory_usage = (struct mem_usage)
    {
        0
    };
}

void   *mem_allocate(u64 size, enum memory_alloc_tag tag)
{
    if (tag == MEMORY_TAG_UNKNOWN)
    {
        char buf[64];
        mem_size_get_pretty_string(size, buf);
        WARN("Allocating %s using MEMORY_TAG_UNKNOWN.", buf);
    }

    u64 full_alloc_size = sizeof(struct mem_chunk_header) + size;
    void *mem           = platform_alloc(full_alloc_size);
    ASSERT_MSG(mem, "Memory allocation failed.");

    current_memory_usage.total_memory_usage     += full_alloc_size;
    current_memory_usage.tags_memory_usage[tag] += full_alloc_size;

    // Write data in the header
    struct mem_chunk_header *header = mem;
    header->allocated_size = full_alloc_size;
    header->allocated_type = tag;

    void *result = mem + sizeof(struct mem_chunk_header);
    return result;
}

void   *mem_reallocate(void *ptr, u64 size)
{
    struct mem_chunk_header *full_block = ptr - sizeof(struct mem_chunk_header);
    current_memory_usage.total_memory_usage                            -= full_block->allocated_size;
    current_memory_usage.tags_memory_usage[full_block->allocated_type] -= full_block->allocated_size;

    current_memory_usage.total_memory_usage                            += size;
    current_memory_usage.tags_memory_usage[full_block->allocated_type] += size;

    full_block = platform_realloc( full_block, size + sizeof(struct mem_chunk_header) );

    void *result = (void *)( (u64)full_block + sizeof(struct mem_chunk_header) );
    return result;
}

void    mem_free(void   *ptr)
{
    ASSERT_MSG(ptr, "Invalid pointer provided.");

    // Get pointer from the beginning with the header
    void *full_chunk = ptr - sizeof(struct mem_chunk_header);
    (void)full_chunk;

    struct mem_chunk_header *header = full_chunk;
    current_memory_usage.total_memory_usage                        -= header->allocated_size;
    current_memory_usage.tags_memory_usage[header->allocated_type] -= header->allocated_size;

    platform_free(full_chunk);
}

void    mem_memset(void *ptr, u8 data, u64 byte_count)
{
    platform_memset(ptr, data, byte_count);
}

void    mem_memcpy(void *dest, void *src, u64 byte_count)
{
    platform_memcpy(dest, src, byte_count);
}

i32     mem_memcmp(void *a, void *b, u64 byte_count)
{
    return platform_memcmp(a, b, byte_count);
}

void   *mem_memmove(void *a, void *b, u64 byte_count)
{
    return platform_memmove(a, b, byte_count);
}

void    mem_print_memory_usage()
{
    INFO("~~~~~~~~~~ Current memory usage ~~~~~~~~~~");
    INFO(
        "Memory type              |"
        " Memory usage"
        );

    // Printing total memory usage
    {
        char buf[64];
        mem_size_get_pretty_string(current_memory_usage.total_memory_usage, buf);

        INFO("  %s : %s", MEMORY_ALLOC_TOTAL_NAME, buf);
    }

    for (int i = 0; i < MEMORY_TAG_COUNT; i++)
    {
        char buf[64];
        mem_size_get_pretty_string(current_memory_usage.tags_memory_usage[i], buf);

        INFO("  %s : %s", memory_alloc_tag_names[i], buf);
    }
    INFO("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
}

u64    mem_size_get_pretty_string(u64 size, char *buffer)
{
    float val   = (float)size;
    u64 printed = 0;

    if (val > MEMORY_UNIT_GIGA)
    {
        val     = val / MEMORY_UNIT_GIGA;
        printed = sprintf(buffer, "%.2f %s", val, MEMORY_UNIT_GIGA_NAME);
    }
    else if (val > MEMORY_UNIT_MEGA)
    {
        val     = val / MEMORY_UNIT_MEGA;
        printed = sprintf(buffer, "%.2f %s", val, MEMORY_UNIT_MEGA_NAME);
    }
    else if (val > MEMORY_UNIT_KILO)
    {
        val     = val / MEMORY_UNIT_KILO;
        printed = sprintf(buffer, "%.2f %s", val, MEMORY_UNIT_KILO_NAME);
    }
    else
    {
        printed = sprintf(buffer, "%.2f %s", val, MEMORY_UNIT_BYTE_NAME);
    }

    return printed;
}

