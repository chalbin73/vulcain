#include "pool_allocator.h"

void    mem_pool_create(mem_pool *pool, void *memory, u64 chunk_size, u64 chunk_count)
{
    pool->memory           = memory;
    pool->chunk_count      = chunk_count;
    pool->chunk_size       = chunk_size;
    pool->free_chunk_count = chunk_count;
    pool->start            = memory;

    mem_memset(pool->memory, 0x00, chunk_count * chunk_size);

    // pool_empy_chunk_header_t *prev = memory;
    // Write empty header
    for (int i = 0; i < pool->chunk_count; i++)
    {
        void *ptr = &( (u8 *)pool->memory )[i * pool->chunk_size];

        mem_pool_empy_chunk_header *node = (mem_pool_empy_chunk_header *)ptr;
        // Push free node onto thte free list
        node->next  = pool->start;
        pool->start = node;
    }
}

void    mem_pool_create_sized_count(mem_pool *pool, void *memory, u64 memory_size, u64 chunk_count)
{
    u64 chunk_size = memory_size / chunk_count; // Euclidean quotient
    mem_pool_create(pool, memory, chunk_size, chunk_count);
}

void    mem_pool_create_sized_size(mem_pool *pool, void *memory, u64 memory_size, u64 chunk_size)
{
    u64 chunk_count = memory_size / chunk_size; // Euclidean quotient
    mem_pool_create(pool, memory, chunk_size, chunk_count);
}

void   *mem_pool_alloc(mem_pool   *pool)
{
    if (pool->free_chunk_count == 0)
        return 0;
    void *res = pool->start;
    pool->start = pool->start->next;
    pool->free_chunk_count--;
    return res;
}

void    mem_pool_dealloc(mem_pool *pool, void *ptr)
{
    pool->free_chunk_count++;
    *( (mem_pool_empy_chunk_header *)ptr ) = (mem_pool_empy_chunk_header)
    {
        .next = pool->start
    };
    pool->start = ptr;
}

void   *mem_pool_destroy(mem_pool   *pool)
{
    void *res = pool->memory;
    *pool = (mem_pool)
    {
        0
    };
    return res;
}

void    mem_pool_get_alloced_blocks(mem_pool *pool, void **block_list, u32 *count)
{
    *count = pool->chunk_count - pool->free_chunk_count;
    if (!block_list)
        return;
    if (count == 0)
        return;

    b8 *is_free = alloca(sizeof(b8) * pool->chunk_count);
    mem_memset(is_free, 0, sizeof(b8) * pool->chunk_count);

    mem_pool_empy_chunk_header *prev = pool->start;
    for (int i = 0; i < pool->free_chunk_count; i++)
    {
        u64 chunk_index = ( ( (u64)prev - (u64)pool->memory ) / (u64)pool->chunk_size );
        is_free[chunk_index] = TRUE;
        prev                 = prev->next;
    }

    u32 c = 0;
    for (int i = 0; i < pool->chunk_count; i++)
    {
        if (!is_free[i])
        {
            block_list[c] = (void *)( (u64)pool->memory + ( (u64)pool->chunk_size * i ) );
            c++; // LOL BJARNE GOES BRRRRRRR
        }
    }
}

void   *mem_pool_index(mem_pool *pool, u64 index)
{
    if(index > pool->chunk_count)
    {
        return NULL;
    }
    return (void *)( (u64)pool->memory + (pool->chunk_size * index) );
}

