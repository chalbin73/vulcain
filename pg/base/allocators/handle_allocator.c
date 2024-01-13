#include "handle_allocator.h"

u64    handle_mgr_get_size_for_count(u64 concurrent_alloc_count, u64 managed_size)
{
    return concurrent_alloc_count * ( managed_size + sizeof(handle_mgr_block_header) );
}

b8     handle_mgr_create(handle_mgr *mgr, void *memory, u64 memory_size, u64 managed_size)
{
    mgr->managed_size = managed_size;
    mgr->memory_size  = memory_size;
    mgr->memory       = memory;

    mgr->block_size     = sizeof(handle_mgr_block_header) + managed_size;
    mgr->block_count    = memory_size / mgr->block_size; // Euclidean quotient
    mgr->alloced_blocks = 0;

    void *ptr = memory;
    for (int i = 0; i < mgr->block_count; i++)
    {
        handle_mgr_block_header hdr =
        {
            .unique_counter = 0,
            .free           = TRUE,
        };

        (*(handle_mgr_block_header *)ptr) = hdr;

        void **ll_ptr = (void **)( (u64)ptr + sizeof(handle_mgr_block_header) );
        ptr     = (void *)( (u64)ptr + mgr->block_size );
        *ll_ptr = (i == mgr->block_count - 1) ? NULL : ptr;
    }

    mgr->first_free_block = memory;

    return TRUE;
}

handle    handle_mgr_allocate(handle_mgr   *mgr)
{
    if (mgr->first_free_block)
    {
        handle_mgr_block_header *hdr = mgr->first_free_block;
        ASSERT_MSG(hdr, "No more free blocks.");
        ASSERT_MSG(hdr->free, "Tried to alloc non free block.");
        hdr->free = FALSE;
        hdr->unique_counter++;
        if (hdr->unique_counter == 0) // Overflow case
        {
            hdr->unique_counter = 1; // Never have a unique counter of 0, otherwise handles could collide with NULL_HANDLE
        }

        // Get where the free block was pointing to
        void **next = (void **)( (u64)hdr + sizeof(handle_mgr_block_header) );
        mgr->first_free_block = *next;

        // Compose
        u16 idx     = ( (u64)hdr - (u64)mgr->memory ) / mgr->block_size;
        handle hndl = idx | (hdr->unique_counter << 16);
        mgr->alloced_blocks++;
        return hndl;
    }
    return NULL_HANDLE;
}

b8    handle_mgr_free(handle_mgr *mgr, handle hndl)
{
    if (hndl)
    {
        handle_mgr_block_header *hdr = (handle_mgr_block_header *)( ( (hndl & 0x00000FFFF) * mgr->block_size ) + (u64)mgr->memory );

        if (hdr->free)
            return FALSE;

        hdr->free = TRUE;
        mgr->alloced_blocks--;

        void **next = (void **)( (u64)hdr + sizeof(handle_mgr_block_header) );
        *next                 = mgr->first_free_block;
        mgr->first_free_block = hdr;

        return TRUE;
    }
    return FALSE;
}

void   *handle_mgr_deref(handle_mgr *mgr, handle hndl)
{
    if (hndl)
    {
        handle_mgr_block_header *hdr = (handle_mgr_block_header *)( ( (hndl & 0x00000FFFF) * mgr->block_size ) + (u64)mgr->memory );

        if (hdr->free)
            return NULL;
        if ( hdr->unique_counter != ( (hndl >> 16) & 0x00000FFFF ) )
            return NULL;

        void *ret = (void *)( (u64)hdr + sizeof(handle_mgr_block_header) );
        return ret;
    }
    return NULL;
}

u64     handle_mgr_get_count(handle_mgr   *mgr)
{
    return mgr->alloced_blocks;
}

void   *handle_mgr_destroy(handle_mgr   *mgr)
{
    void *tmp = mgr->memory;
    mgr->memory_size = 0;
    mgr->memory      = NULL;
    return tmp;
}

