#pragma once

//Simple allocator that allocates/deallocates chunks at a time

#include "../types.h"
#include "../base.h"


typedef struct mem_pool_empy_chunk_header
{
    struct mem_pool_empy_chunk_header *next;
} mem_pool_empy_chunk_header;

typedef struct
{
    void *memory;
    u64 chunk_size;
    u64 chunk_count;

    u64 free_chunk_count;

    //Linked list of pointers of empty chunks
    mem_pool_empy_chunk_header *start;
} mem_pool;

void mem_pool_create(mem_pool *pool, void *memory, u64 chunk_size, u64 chunk_count);
void mem_pool_create_sized_count(mem_pool *pool, void *memory, u64 memory_size, u64 chunk_count);
void mem_pool_create_sized_size(mem_pool *pool, void *memory, u64 memory_size, u64 chunk_size);
void *mem_pool_alloc(mem_pool *pool);
void mem_pool_dealloc(mem_pool *pool, void* ptr);
void *mem_pool_destroy(mem_pool *pool);
void mem_pool_get_alloced_blocks(mem_pool *pool, void **block_list, u32 *count);