#include "../base/base.h"
#include "../base/allocators/handle_allocator.h"

typedef enum
{
    VC_HANDLE_TYPE_COUNT
} vc_handle_type;


#define VC_DEF_HANDLE(handle_name) \
    typedef union { u64 value; struct { handle hndl; vc_handle_type type; }; } handle_name;


// Anonymous handle, used for casting in manager
VC_DEF_HANDLE(vc_handle);

#define VC_NULL_HANDLE (vc_handle){0}

typedef struct
{
    handle_mgr handle_managers[VC_HANDLE_TYPE_COUNT];
} vc_handle_mgr;

typedef u64 vc_handle_mgr_sizes[VC_HANDLE_TYPE_COUNT];

b8 vc_handle_mgr_create(vc_handle_mgr *mgr, vc_handle_mgr_sizes sizes);

vc_handle vc_handle_mgr_alloc(vc_handle_mgr *mgr, vc_handle_type type);
b8 vc_handle_mgr_free(vc_handle_mgr *mgr, vc_handle hndl);

void vc_handle_mgr_destroy(vc_handle_mgr *mgr);