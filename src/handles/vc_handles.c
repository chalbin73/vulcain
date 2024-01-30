/*
 * In this .h/.c is implemented the logic for the handles exposed by the api of vulcain
 * Handles should be typed, and infered by this module. Thus, generic handles could be used
 *
 */

#include "vc_handles.h"
#include "vc_internal_types.h"
#include "../vulcain.h"
#include "../base/data_structures/darray.h"

static const u64 _vc_struct_sizes[VC_HANDLE_TYPES_COUNT] =
{
    [VC_HANDLE_SWAPCHAIN] = sizeof(_vc_swapchain_intern),
    [VC_HANDLE_QUEUE]     = sizeof(_vc_queue_intern),
};

static const u64 _vc_initial_chunk_counts[VC_HANDLE_TYPES_COUNT] =
{
    [VC_HANDLE_SWAPCHAIN] = 8,
    [VC_HANDLE_QUEUE]     = 8,
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

void
vc_handles_manager_create(vc_handles_manager   *mgr)
{
    for(u32 i = 0; i < VC_HANDLE_TYPES_COUNT; i++)
    {
        vc_handle_pool_create(&mgr->pools[i], _vc_initial_chunk_counts[i], _vc_struct_sizes[i]);
    }

    mgr->destroy_queue = darray_create(vc_handle);
}

void
vc_handles_manager_set_destroy_function(vc_handles_manager *mgr, vc_handle_type hndl_type, vc_handle_destroy_func func)
{
    mgr->destroy_functions[hndl_type] = func;
}

void
vc_handles_manager_set_destroy_function_usr_data(vc_handles_manager *mgr, void *usr_data)
{
    mgr->dest_func_usr_data = usr_data;
}

void
vc_handles_manager_destroy(vc_handles_manager   *mgr)
{
    u32 length = darray_length(mgr->destroy_queue);
    for(u32 i = 0; i < length; i++)
    {
        vc_handle hndl;
        darray_pop(mgr->destroy_queue, &hndl);

        vc_handle_pack pck;
        pck.vc_hndl = hndl;

        if(mgr->destroy_functions[pck.type] != NULL)
        {
            void *obj = vc_handles_manager_deref(mgr, hndl);
            mgr->destroy_functions[pck.type](mgr->dest_func_usr_data, obj, pck.type);
        }
    }

    // Destroy everything
    for(u32 i = 0; i < VC_HANDLE_TYPES_COUNT; i++)
    {
        vc_handle_pool_destroy(&mgr->pools[i]);
    }

    darray_destroy(mgr->destroy_queue);
    mgr->destroy_queue = NULL;
}

void *
vc_handles_manager_deref(vc_handles_manager *mgr, vc_handle hndl)
{
    // Unpack
    vc_handle_pack pck;
    pck.vc_hndl = hndl;

    if(pck.type >= VC_HANDLE_TYPES_COUNT)
    {
        vc_error("Attempted to dereference an invalid vc_handle (invalid type).");
        return NULL;
    }

    // Dereference in underlying pool
    // Invalid handle within the pool is handled in the pool
    void *ptr = vc_handle_pool_deref(&mgr->pools[pck.type], pck.id_hndl);

    return ptr;
}

vc_handle
vc_handles_manager_alloc(vc_handles_manager *mgr, vc_handle_type type)
{
    if(type >= VC_HANDLE_TYPES_COUNT)
    {
        vc_error("Attempted to alloc vc_handle from invalid type.");
        return VC_NULL_HANDLE;
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

void
vc_handles_manager_dealloc(vc_handles_manager *mgr, vc_handle hndl)
{
    vc_handle_pack pck;
    pck.vc_hndl = hndl;

    if(pck.type >= VC_HANDLE_TYPES_COUNT)
    {
        vc_error("Attempted to free an invalid vc_handle (invalid type).");
        return;
    }

    vc_handle_pool_dealloc(&mgr->pools[pck.type], pck.id_hndl);

    // Search for handle in pool, and remove it
    {
        u32 length = darray_length(mgr->destroy_queue);
        b8 found   = FALSE;
        for(u32 i = 0; i < length; i++)
        {
            if(mgr->destroy_queue[i] == hndl)
            {
                // Handle was found
                darray_pop_at(mgr->destroy_queue, i, NULL);
                found = TRUE;
                break;
            }
        }

        if(!found)
        {
            vc_warn("A dealloced handle (%x) was not found in the destroy queue.", hndl);
        }
    }
    return;
}

// Frees the handle and destroys the underlying object, according to the linked function
void
vc_handles_manager_destroy_handle(vc_handles_manager *mgr, vc_handle hndl)
{
    void *obj = vc_handles_manager_deref(mgr, hndl);

    if(obj == NULL)
    {
        vc_error("Attempted to destroy an invalid vc_handle (null reference).");
        return;
    }

    vc_handle_pack pck;
    pck.vc_hndl = hndl;

    if(pck.type >= VC_HANDLE_TYPES_COUNT)
    {
        vc_error("Attempted to destroy an invalid vc_handle (invalid type).");
        return;
    }

    if(mgr->destroy_functions[pck.type] != NULL)
    {
        mgr->destroy_functions[pck.type](mgr->dest_func_usr_data, obj, pck.type);
    }

    vc_handles_manager_dealloc(mgr, hndl);
}

