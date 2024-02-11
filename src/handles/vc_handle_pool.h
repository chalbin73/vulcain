/*
   This library uses a handle system, rather than a pointer system.
   In order to reduce memory issues caused by the users, instead of
   handing pointers to the outside, opaque, 32-bit numbers are handed
   as handles, that can be used through the api in order to reference objects.

   In this .h, which should not be exposed to the outside api, implements
   the generic pool into which objects will be stored.
 */

#ifndef __VC_HANDLE_POOL__
#define __VC_HANDLE_POOL__

#include "../base/base.h"

typedef struct
{
    u16    chunk_counter;
    b8     used;

    // Next index
    // Located into data
    u64    next_id;
} vc_handle_pool_chunk_header;

typedef struct
{
    void   *memory;
    u64     head_id;
    u64     chunk_count;
    u64     available_count;
    u64     chunk_size;
}vc_handle_pool;

typedef union
{
    u32    hndl_id;

    struct
    {
        u16    index;
        u16    counter;
    };
} vc_handle_mask;

void    vc_handle_pool_create(vc_handle_pool *pool, u64 initial_chunk_count, u64 managed_size);
void    vc_handle_pool_destroy(vc_handle_pool   *pool);

u32     vc_handle_pool_alloc(vc_handle_pool   *pool);
void   *vc_handle_pool_deref(vc_handle_pool *pool, u32 id);
void    vc_handle_pool_dealloc(vc_handle_pool *pool, u32 id);

#endif // __VC_HANDLE_POOL__

