#include "../vulcain.h"
#include "vc_handle_pool.h"

#define _VC_HANDLE_POOL_NULL_NEXT ( ~(0L) )

void
vc_handle_pool_create(vc_handle_pool *pool, u64 initial_chunk_count, u64 managed_size)
{
    if(initial_chunk_count == 0 || managed_size == 0)
    {
        vc_error("Pool creation called with invalid parameters");
        pool->memory = NULL;
        return;
    }

    pool->chunk_size = managed_size + ( sizeof(vc_handle_pool_chunk_header) - sizeof(u64) );
    u64 initial_size = pool->chunk_size * initial_chunk_count;
    pool->memory = mem_allocate(initial_size, MEMORY_TAG_RENDER_DATA);

    // Set up headers
    for(u32 i = 0; i < initial_chunk_count; i++)
    {
        vc_handle_pool_chunk_header hdr =
        {
            .used          = FALSE,
            .chunk_counter = 0, // 0 chunks are considered as NULL/ special value, as they are incremented at first alloc
            .next_id       = i + 1,
        };

        *( (vc_handle_pool_chunk_header *)( (u8 *)pool->memory + pool->chunk_size * i ) ) = hdr;

        if(i == initial_chunk_count - 1)
        {
            hdr.next_id = _VC_HANDLE_POOL_NULL_NEXT;
        }
    }

    pool->chunk_count     = initial_chunk_count;
    pool->available_count = initial_chunk_count;
    pool->head_id         = 0;
}

void
vc_handle_pool_destroy(vc_handle_pool   *pool)
{
    mem_free(pool->memory);
    pool->memory          = NULL;
    pool->chunk_count     = 0;
    pool->available_count = 0;
}

u32
vc_handle_pool_alloc(vc_handle_pool   *pool)
{
    if(pool->chunk_count == 0 || pool->memory == NULL)
    {
        vc_error("Pool allocation called on invalid pool. Aborting.");
        return 0;
    }
    if(pool->available_count == 0)
    {
        vc_warn("Full pull, allocation was requested, can't fullfill.");
        return 0; // Maybe handle resize here
    }

    // Grab new zone from head
    u32 new_id = pool->head_id;

    // Dereference pool
    vc_handle_pool_chunk_header *new_hdr = (vc_handle_pool_chunk_header *)( ( (u8 *)pool->memory ) + (new_id * pool->chunk_size) );
    if(new_hdr->used)
    {
        vc_fatal("Pool in free list is used.");
        return 0;
    }

    new_hdr->used = TRUE;
    new_hdr->chunk_counter++;

    // Pull new out from list
    pool->head_id = new_hdr->next_id;

    vc_handle_mask new_hndl = (vc_handle_mask)
    {
        .index   = new_id,
        .counter = new_hdr->chunk_counter,
    };
    pool->available_count--;

    return new_hndl.hndl_id;
}

void *
vc_handle_pool_deref(vc_handle_pool *pool, u32 id)
{
    if(id == 0)
    {
        vc_error("Attempted to dereference a Null (0) handle id.");
        return NULL;
    }
    vc_handle_mask mask =
    {
        0
    };
    mask.hndl_id = id;

    vc_handle_pool_chunk_header *hdr = (vc_handle_pool_chunk_header *)( ( (u8 *)pool->memory ) + (pool->chunk_size * mask.index) );

    if(!hdr->used)
    {
        vc_error("Attempted to access a free ressource. Dangling access is possible.");
        return NULL;
    }

    if(hdr->chunk_counter != mask.counter)
    {
        vc_error("Handle (id=0x%x) counter mismatch with managed ressource (%d != %d). This indicate a dangling access.", id, hdr->chunk_counter, mask.counter);
        return NULL;
    }

    return &hdr->next_id; // Next id is only used when the chunk is empty. Data is written over it otherwise
}

void
vc_handle_pool_dealloc(vc_handle_pool *pool, u32 id)
{
    if(id == 0)
    {
        vc_error("Attempted to dealloc a Null (0) handle id.");
        return;
    }
    vc_handle_mask mask =
    {
        0
    };
    mask.hndl_id = id;

    vc_handle_pool_chunk_header *hdr = pool->memory + (pool->chunk_count * mask.index);

    if(hdr->chunk_counter != mask.counter)
    {
        vc_error("Handle counter mismatch with managed ressource. This indicate a dangling deallocation.");
        return;
    }

    pool->available_count++;
    // We now that we are trying to dealloc the correct chunk

    // Push chunk into linked list
    hdr->used     = FALSE;
    hdr->next_id  = pool->head_id;
    pool->head_id = mask.index;
}

