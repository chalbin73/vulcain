#pragma once

//NOTE: Contains memory allocators for the engine

#include "types.h"
#include "logging.h"
#include "platform.h"

// Units
#define MEMORY_UNIT_BYTE 1
// NOTE: Can be changed to use KB/MB/GB (1000) or KiB/MiB/GiB (1024) 
#define MEMORY_UNIT_KILO 1024
#define MEMORY_UNIT_MEGA (MEMORY_UNIT_KILO * MEMORY_UNIT_KILO)
#define MEMORY_UNIT_GIGA (MEMORY_UNIT_MEGA * MEMORY_UNIT_KILO)

#define MEMORY_UNIT_BYTE_NAME "B"
#define MEMORY_UNIT_KILO_NAME "KiB" 
#define MEMORY_UNIT_MEGA_NAME "MiB"
#define MEMORY_UNIT_GIGA_NAME "GiB"

#include "memory_tags.h"

#define MEMORY_ALLOC_TOTAL_NAME "MEMORY_TOTAL      "

struct mem_usage
{
    u64 total_memory_usage;
    u64 tags_memory_usage[MEMORY_TAG_COUNT];
};

//This struct is written at the beginning of every allocated memory chunk
struct mem_chunk_header
{
    //The number of bytes that are allocated
    u64 allocated_size;

    //The type of memory that is allocated
    u32 allocated_type;
};

__attribute__((unused))
static struct mem_usage current_memory_usage;

void memory_start();
void memory_shutdown();

void *mem_allocate(u64 size, enum memory_alloc_tag tag);
void *mem_reallocate(void *ptr, u64 size);
void mem_free(void *ptr);

void mem_memset(void *ptr, u8 data, u64 byte_count);
void mem_memcpy(void *dest, void *src, u64 byte_count);

void mem_print_memory_usage();

// Converts a u64 size into a human readable format (3.44KiB, 4.25GiB ...)
u64 mem_size_get_pretty_string(u64 size, char *buffer);