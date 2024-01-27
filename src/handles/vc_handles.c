/*
 * In this .h/.c is implemented the logic for the handles exposed by the api of vulcain
 * Handles should be typed, and infered by this module. Thus, generic handles could be used
 *
 */

#include "vc_handles.h"
#include "vc_internal_types.h"
#include "../vulcain.h"

static const u64 _vc_struct_sizes[VC_HANDLE_TYPES_COUNT] =
{
    [VC_HANDLE_SWAPCHAIN] = sizeof(_vc_swapchain_intern),
};

static const u64 _vc_initial_chunk_counts[VC_HANDLE_TYPES_COUNT] =
{
    [VC_HANDLE_SWAPCHAIN] = 8,
};

typedef union
{
    vc_handle    vc_hndl;
    struct
    {
        vc_handle_type    type; // Type to dispatch to pool
        u32               id_hndl; // Id in underlying pool
    };
} vc_handle_pack;

void    vc_handles_manager_create(vc_handles_manager   *mgr)
{
    for(u32 i = 0; i < VC_HANDLE_TYPES_COUNT; i++)
    {
        vc_handle_pool_create(&mgr->pools[i], _vc_initial_chunk_counts[i], _vc_struct_sizes[i]);
    }
}

void   *vc_handles_manager_deref(vc_handles_manager *mgr, vc_handle hndl)
{
    // Unpack
    vc_handle_pack pck;
    pck.vc_hndl = hndl;

    if(pck.type == 0 || pck.type >= VC_HANDLE_TYPES_COUNT)
    {
        vc_error("Attempted to dereference an invalid vc_handle (invalid type).");
        return NULL;
    }

    // Dereference in underlying pool
    // Invalid handle within the pool is handled in the pool
    void *ptr = vc_handle_pool_deref(&mgr->pools[pck.type], pck.id_hndl);

    // Remove from destroy queue
    {

    }

    return ptr;
}

vc_handle    vc_handles_manager_alloc(vc_handles_manager *mgr, vc_handle_type type)
{
    if(type == 0 || type >= VC_HANDLE_TYPES_COUNT)
    {
        vc_error("Attempted to alloc vc_handle from invalid type.");
        return NULL;
    }
    u32 id_hndl = vc_handle_pool_alloc(&mgr->pools[type]);


    // Pack handle
    vc_handle_pack pck;
    pck.type    = type;
    pck.id_hndl = id_hndl;

    // Maintain destroy queue
    darray_push(mgr->destroy_queue, pck.vc_hndl);

    return pck.vc_hndl;
}

