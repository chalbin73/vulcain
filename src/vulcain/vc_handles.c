#include "vc_handles.h"
#include "vc_managed_types.h"

b8    vc_handle_mgr_create(vc_handle_mgr *mgr, vc_handle_mgr_counts counts)
{
    static const vc_handle_mgr_counts managed_sizes =
    {
        [VC_HANDLE_COMPUTE_PIPE]          = sizeof(vc_priv_man_compute_pipe),
        [VC_HANDLE_GRAPHICS_PIPE]         = sizeof(vc_priv_man_graphics_pipe),
        [VC_HANDLE_COMMAND_BUFFER]        = sizeof(vc_priv_man_command_buffer),
        [VC_HANDLE_SEMAPHORE]             = sizeof(vc_priv_man_semaphore),
        [VC_HANDLE_IMAGE]                 = sizeof(vc_priv_man_image),
        [VC_HANDLE_BUFFER]                = sizeof(vc_priv_man_buffer),
        [VC_HANDLE_DESCRIPTOR_SET_LAYOUT] = sizeof(vc_priv_man_descriptor_set_layout),
        [VC_HANDLE_DESCRIPTOR_SET]        = sizeof(vc_priv_man_descriptor_set),
        [VC_HANDLE_RENDER_PASS]           = sizeof(vc_priv_man_render_pass),
        [VC_HANDLE_FRAMEBUFFER]           = sizeof(vc_priv_man_framebuffer),
    };

    mem_memcpy( mgr->sizes, (void *)managed_sizes, sizeof(vc_handle_mgr_counts) );
    mem_memcpy( mgr->counts, (void *)counts, sizeof(vc_handle_mgr_counts) );

    for (int i = 0; i < VC_HANDLE_TYPE_COUNT; i++)
    {
        u64 mem_size = handle_mgr_get_size_for_count(counts[i], managed_sizes[i]);
        handle_mgr_create(&mgr->handle_managers[i], mem_allocate(mem_size, MEMORY_TAG_RENDERER), mem_size, managed_sizes[i]);
    }

    // Setup destruction queue
    mgr->destroy_queue = darray_create(vc_handle);

    return TRUE;
}

vc_handle    vc_handle_mgr_alloc(vc_handle_mgr *mgr, vc_handle_type type)
{
    ASSERT_MSG(type < VC_HANDLE_TYPE_COUNT, "Invalide handle type provided.");

    handle hndl = handle_mgr_allocate(&mgr->handle_managers[type]);
    DEBUG( "Allocating a %s", vc_handle_type_to_str(type) );
    if (hndl)
    {
        vc_handle_unpack vc_hndl =
        {
            .type = type,
            .hndl = hndl,
        };
        darray_push(mgr->destroy_queue, vc_hndl);
        return vc_hndl.packed;
    }
    WARN("Null handle acquired, type='%d'", type);
    return VC_NULL_HANDLE;
}

void         vc_handle_mgr_set_destroy_func(vc_handle_mgr *mgr, vc_handle_type type, vc_man_destroy_func func)
{
    mgr->destroy_funcs[type] = func;
}

vc_handle    vc_handle_mgr_write_alloc(vc_handle_mgr *mgr, vc_handle_type type, void *object)
{
    vc_handle hndl = vc_handle_mgr_alloc(mgr, type);
    vc_handle_mgr_write(mgr, hndl, object);
    return hndl;
}

b8           vc_handle_mgr_free(vc_handle_mgr *mgr, vc_handle hndl, void *destroy_ctx)
{
    vc_handle_unpack unpacked =
    {
        .packed = hndl
    };
    ASSERT_MSG(unpacked.packed, "Invalid handle provided.");
    ASSERT_MSG(unpacked.type < VC_HANDLE_TYPE_COUNT, "Invalide handle type provided.");

    DEBUG( "Freeing a %s", vc_handle_type_to_str(unpacked.type) );

    // Remove from destruction queue (search)
    u32 count = darray_length(mgr->destroy_queue);
    for (int i = 0; i < count; i++)
    {
        if (mgr->destroy_queue[i] == hndl)
        {
            // Handle found, remove
            darray_pop_at(mgr->destroy_queue, i, NULL);
            break;
        }
    }

    // Call destroy function
    if (mgr->destroy_funcs[unpacked.type])
    {
        mgr->destroy_funcs[unpacked.type]( destroy_ctx, vc_handle_mgr_ptr(mgr, hndl) );
    }

    return handle_mgr_free(&mgr->handle_managers[unpacked.type], unpacked.hndl);
}

void    vc_handle_mgr_write(vc_handle_mgr *mgr, vc_handle h, void *object)
{
    vc_handle_unpack hndl =
    {
        .packed = h
    };
    void *dest = vc_handle_mgr_ptr(mgr, h);
    mem_memcpy(dest, object, mgr->sizes[hndl.type]);
}

void   *vc_handle_mgr_ptr(vc_handle_mgr *mgr, vc_handle h)
{
    vc_handle_unpack hndl =
    {
        .packed = h
    };

    ASSERT_MSG(hndl.type < VC_HANDLE_TYPE_COUNT, "Invalid handle provided (out of bounds type)");
    ASSERT_MSG(h != VC_NULL_HANDLE, "Invalid handle provided (NULL HANDLE)");

    return handle_mgr_deref(&mgr->handle_managers[hndl.type], hndl.hndl);
}

void    vc_handle_mgr_destroy(vc_handle_mgr *mgr, void *destroy_ctx)
{
    u32 count = darray_length(mgr->destroy_queue);
    for (int i = count - 1; i >= 0; i--)
    {
        vc_handle_unpack hndl =
        {
            .packed = mgr->destroy_queue[i]
        };
        if (mgr->destroy_funcs[hndl.type])
        {
            mgr->destroy_funcs[hndl.type]( destroy_ctx, vc_handle_mgr_ptr(mgr, hndl.packed) );
        }
        else
        {
            WARN( "No destroy function registered for type %s", vc_handle_type_to_str(hndl.type) );
        }
    }

    for (int i = 0; i < VC_HANDLE_TYPE_COUNT; i++)
    {
        handle_mgr_destroy(&mgr->handle_managers[i]);
    }

    darray_destroy(mgr->destroy_queue);
}

const char   *vc_handle_type_to_str(vc_handle_type    type)
{
    switch (type)
    {
    case VC_HANDLE_COMPUTE_PIPE:
        return "VC_HANDLE_COMPUTE_PIPE";
    case VC_HANDLE_GRAPHICS_PIPE:
        return "VC_HANDLE_GRAPHICS_PIPE";
    case VC_HANDLE_COMMAND_BUFFER:
        return "VC_HANDLE_COMMAND_BUFFER";
    case VC_HANDLE_SEMAPHORE:
        return "VC_HANDLE_SEMAPHORE";
    case VC_HANDLE_IMAGE:
        return "VC_HANDLE_IMAGE";
    case VC_HANDLE_BUFFER:
        return "VC_HANDLE_BUFFER";
    case VC_HANDLE_DESCRIPTOR_SET_LAYOUT:
        return "VC_HANDLE_DESCRIPTOR_SET_LAYOUT";
    case VC_HANDLE_DESCRIPTOR_SET:
        return "VC_HANDLE_DESCRIPTOR_SET";
    case VC_HANDLE_RENDER_PASS:
        return "VC_HANDLE_RENDER_PASS";
    case VC_HANDLE_FRAMEBUFFER:
        return "VC_HANDLE_FRAMEBUFFER";


    case VC_HANDLE_TYPE_COUNT:
        break;

    }
    return "Unknown handle type";
}