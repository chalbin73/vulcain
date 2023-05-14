#include "vc_handles.h"

b8 vc_create_handle_mgr(vc_handle_mgr *mgr, vc_handle_mgr_sizes sizes)
{
    static const vc_handle_mgr_sizes managed_sizes = 
    {

    };

    for(int i = 0; i < VC_HANDLE_TYPE_COUNT; i++)
    {
        u64 mem_size = handle_mgr_get_size_for_count(sizes[i], managed_sizes[i]);
        handle_mgr_create(&mgr->handle_managers[i], mem_allocate(mem_size, MEMORY_TAG_RENDERER), mem_size, managed_sizes[i]);
    }

    return TRUE;
}

vc_handle vc_handle_mgr_alloc(vc_handle_mgr *mgr, vc_handle_type type)
{
    ASSERT_MSG(type < VC_HANDLE_TYPE_COUNT, "Invalide handle type provided.");

    handle hndl = handle_mgr_allocate(&mgr->handle_managers[type]);
    
    if(hndl)
    {
        vc_handle_unpack vc_hndl =
        {
            .type = type,
            .hndl = hndl
        };
        return vc_hndl.packed;
    }
    return VC_NULL_HANDLE;
}

b8 vc_handle_mgr_free(vc_handle_mgr *mgr, vc_handle hndl)
{   
    vc_handle_unpack unpacked = {.packed = hndl};
    ASSERT_MSG(unpacked.packed, "Invalid handle provided.");
    ASSERT_MSG(unpacked.type < VC_HANDLE_TYPE_COUNT, "Invalide handle type provided.");

    return handle_mgr_free(&mgr->handle_managers[unpacked.type], unpacked.hndl);
}

void vc_destroy_handle_mgr(vc_handle_mgr *mgr)
{
    for(int i = 0; i < VC_HANDLE_TYPE_COUNT; i++)
    {
        handle_mgr_destroy(&mgr->handle_managers[i]);
    }
}