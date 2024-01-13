#pragma once

// NOTE: Contains memory allocators for the engine

#include "logging.h"
#include "platform.h"
#include "types.h"

// Units
#define MEMORY_UNIT_BYTE        1
// NOTE: Can be changed to use KB/MB/GB (1000) or KiB/MiB/GiB (1024)
#define MEMORY_UNIT_KILO        1024
#define MEMORY_UNIT_MEGA        (MEMORY_UNIT_KILO * MEMORY_UNIT_KILO)
#define MEMORY_UNIT_GIGA        (MEMORY_UNIT_MEGA * MEMORY_UNIT_KILO)

#define MEMORY_UNIT_BYTE_NAME   "B"
#define MEMORY_UNIT_KILO_NAME   "KiB"
#define MEMORY_UNIT_MEGA_NAME   "MiB"
#define MEMORY_UNIT_GIGA_NAME   "GiB"

#include "memory_tags.h"

#define MEMORY_ALLOC_TOTAL_NAME "MEMORY_TOTAL      "

struct mem_usage
{
    u64    total_memory_usage;
    u64    tags_memory_usage[MEMORY_TAG_COUNT];
};

// This struct is written at the beginning of every allocated memory chunk
struct mem_chunk_header
{
    // The number of bytes that are allocated
    u64    allocated_size;

    // The type of memory that is allocated
    u32    allocated_type;
};

__attribute__( (unused) ) static struct mem_usage current_memory_usage;

/**
 * @brief Start the memory subsystem
 *
 */
void    memory_start();

/**
 * @brief Shut the memory subsystem down
 *
 */
void    memory_shutdown();

/**
 * @brief Allocates a block of memory
 *
 * @param size The size of the block
 * @param tag The tag of the block
 * @return void* A pointer to the beginning of the block
 */
void   *mem_allocate(u64 size, enum memory_alloc_tag tag);

/**
 * @brief Reallocates a block of memory, copying old contents
 *
 * @param ptr The old block
 * @param size The size of the new block
 * @return void* A pointer to the beginning of the new block
 */
void   *mem_reallocate(void *ptr, u64 size);

/**
 * @brief Frees a block of memory
 *
 * @param ptr A pointer to the beginning of the block
 */
void    mem_free(void   *ptr);

/**
 * @brief Set an entire block of memory
 *
 * @param ptr A pointer to the beginning of the block to set
 * @param data The byte to set the block with
 * @param byte_count The number of bytes to set
 */
void    mem_memset(void *ptr, u8 data, u64 byte_count);

/**
 * @brief Copies a block of memory
 *
 * @param dest The destination
 * @param src The source
 * @param byte_count The number of bytes to copy
 * @attention The regions may not overlapp
 */
void    mem_memcpy(void *dest, void *src, u64 byte_count);

/**
 * @brief Compares two bloks of memory
 *
 * @param a The first block of memory
 * @param b The second block of memory
 * @param byte_count The number of bytes to compare
 * @returns 0 if equal, non-zero if different
 */
i32     mem_memcmp(void *a, void *b, u64 byte_count);

/**
 * @brief Moves a block of memory
 *
 * @param a The first block of memory
 * @param b The second block of memory
 * @param byte_count The number of bytes to move
 * @returns a
 */
void   *mem_memmove(void *a, void *b, u64 byte_count);

/**
 * @brief Prints the memory using accoring to each tag
 *
 */
void    mem_print_memory_usage();

/**
 * @brief Converts a `u64` size into a human readable format (3.44KiB, 4.25GiB ...)
 *
 */
u64     mem_size_get_pretty_string(u64 size, char *buffer);

