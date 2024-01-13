#pragma once

// Simple allocator that allocates/deallocates chunks at a time

#include "../base.h"
#include "../types.h"

typedef struct mem_pool_empy_chunk_header
{
    struct mem_pool_empy_chunk_header   *next;
} mem_pool_empy_chunk_header;

/**
 * @brief A memory pool allocator object
 *
 */
typedef struct
{
    void                         *memory;
    u64                           chunk_size;
    u64                           chunk_count;

    u64                           free_chunk_count;

    // Linked list of pointers of empty chunks
    mem_pool_empy_chunk_header   *start;
} mem_pool;

/**
 * @brief Creates a memory pool with a memory block, cut into `chunk_count` chunks of `chunk_size`
 *
 * @param pool A zero-out pool object to create the pool in
 * @param memory The memory block
 * @param chunk_size The size of a chunk
 * @param chunk_count The number of chunk to provide
 */
void    mem_pool_create(mem_pool *pool, void *memory, u64 chunk_size, u64 chunk_count);

/**
 * @brief Creates a memory pool with a memory block, of a at least `memory_size` bytes, and creates biggest chunks possible
 *
 * @param pool A zero-out pool object to create the pool in
 * @param memory The memory block
 * @param memory_size The size of the memory block
 * @param chunk_count The number of chunk to provide
 */
void    mem_pool_create_sized_count(mem_pool *pool, void *memory, u64 memory_size, u64 chunk_count);

/**
 * @brief Creates a memory pool with a memory block, of a at least `memory_size` bytes, and creates most chunks of `chunk_size` possible
 *
 * @param pool A zero-out pool object to create the pool in
 * @param memory The memory block
 * @param memory_size The size of the memory block
 * @param chunk_count The size of the chunks
 */
void    mem_pool_create_sized_size(mem_pool *pool, void *memory, u64 memory_size, u64 chunk_size);

/**
 * @brief Allocates a chunk in the pool
 *
 * @param pool The pool object
 * @return void* A pointer to the allocated chunk, is NULL if no more chunk are available
 */
void   *mem_pool_alloc(mem_pool   *pool);

/**
 * @brief Frees a chunk in the pool
 *
 * @param pool The pool object
 * @param ptr A pointer to the chunk to be deallocated
 */
void    mem_pool_dealloc(mem_pool *pool, void *ptr);

/**
 * @brief Destroys the pool
 *
 * @param pool The pool to destroy
 * @return void* A pointer to the block of memory
 */
void   *mem_pool_destroy(mem_pool   *pool);

/**
 * @brief Get a list of pointer to all allocated block
 *
 * @param pool The pool object
 * @param block_list The list of pointer, can be NULL to query count
 * @param count The number of allocated block
 * @deprecated This function is deprectated
 */
void    mem_pool_get_alloced_blocks(mem_pool *pool, void **block_list, u32 *count);

/**
 * @brief Gets a pointer to a block via its index
 *
 * @param pool The pool object
 * @param index The block's index
 */
void   *mem_pool_index(mem_pool *pool, u64 index);
