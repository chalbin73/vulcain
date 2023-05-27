#pragma once

#include "../base/allocators/handle_allocator.h"
#include "../base/base.h"
#include "../base/data_structures/darray.h"

#define VC_DEF_HANDLE(handle_name) \
    typedef u64 handle_name;

/* ---------------- Handle types ---------------- */
typedef enum
{
    VC_HANDLE_COMPUTE_PIPE,
    VC_HANDLE_COMMAND_BUFFER,
    VC_HANDLE_SEMAPHORE,
    VC_HANDLE_IMAGE,
    VC_HANDLE_TYPE_COUNT
} vc_handle_type;

// Anonymous handle, used for casting in manager
VC_DEF_HANDLE(vc_handle);
VC_DEF_HANDLE(vc_compute_pipe);
VC_DEF_HANDLE(vc_semaphore);
VC_DEF_HANDLE(vc_command_buffer);
VC_DEF_HANDLE(vc_image);

typedef union
{
    vc_handle packed;

    struct
    {
        handle         hndl;
        vc_handle_type type;
    };
} vc_handle_unpack;

#define VC_NULL_HANDLE 0L

// Associates an u64 to each managed type, used to store the size of managed type, or max count
typedef u64 vc_handle_mgr_counts[VC_HANDLE_TYPE_COUNT];

// Function that is executed on each object upon destroying handle mgr
typedef b8 (*vc_man_destroy_func)(void *ctx, void *managed_object);

typedef struct
{
    handle_mgr           handle_managers[VC_HANDLE_TYPE_COUNT];
    vc_man_destroy_func  destroy_funcs[VC_HANDLE_TYPE_COUNT];
    vc_handle           *destroy_queue; // darray
    vc_handle_mgr_counts sizes;
    vc_handle_mgr_counts counts;
} vc_handle_mgr;

b8   vc_handle_mgr_create(vc_handle_mgr *mgr, vc_handle_mgr_counts counts);
void vc_handle_mgr_set_destroy_func(vc_handle_mgr *mgr, vc_handle_type type, vc_man_destroy_func func);

vc_handle vc_handle_mgr_alloc(vc_handle_mgr *mgr, vc_handle_type type);
void      vc_handle_mgr_write(vc_handle_mgr *mgr, vc_handle hndl, void *object);
vc_handle vc_handle_mgr_write_alloc(vc_handle_mgr *mgr, vc_handle_type type, void *object);
void     *vc_handle_mgr_ptr(vc_handle_mgr *mgr, vc_handle hndl);
b8        vc_handle_mgr_free(vc_handle_mgr *mgr, vc_handle hndl, void *destroy_ctx);

void vc_handle_mgr_destroy(vc_handle_mgr *mgr, void *destroy_ctx);
