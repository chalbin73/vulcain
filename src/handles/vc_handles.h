#ifndef __VC_HANDLES__
#define __VC_HANDLES__

#include "../base/types.h"
#include "vc_handle_pool.h" // TODO: Figure out a neat way to not include this
#include "../base/data_structures/darray.h"

typedef enum
{
    VC_HANDLE_SWAPCHAIN = 1,
    VC_HANDLE_TYPES_COUNT,
} vc_handle_type;

typedef u64                  vc_handle;

#define VC_DEF_HANDLE(hndl) \
        typedef vc_handle    hndl;


VC_DEF_HANDLE(vc_swapchain);

typedef struct
{
    vc_handle_pool    pools[VC_HANDLE_TYPES_COUNT];
    vc_handle        *destroy_queue; // We maintain a destroy queue, to destroy, in order of creation
} vc_handles_manager;

#endif // __VC_HANDLES__

