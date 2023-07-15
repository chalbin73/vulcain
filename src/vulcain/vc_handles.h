#pragma once

#include "../base/allocators/handle_allocator.h"
#include "../base/base.h"
#include "../base/data_structures/darray.h"

/**
 * @brief Handle system used to manage all the objects created via vulcain, as well as their destruction
 *
 */

/*
   TODO:
   I don't really like how this is separated fully from the vulcain header
   this translation unit does define important handles and functions used in the vulcain system,
   but knows nothing about vc_ctx, thus the destroy function only takes a pointer:
   This feels weird
 */

#define VC_DEF_HANDLE(handle_name) \
    typedef u64 handle_name;

/*
   Each vulcain handle is a 64 bit word, made up of
   - 32 bits : the handle in the handle pool of the type
   - 32 bits : the type of the handle
 */

/* ---------------- Handle types ---------------- */
typedef enum
{
    VC_HANDLE_COMPUTE_PIPE,
    VC_HANDLE_COMMAND_BUFFER,
    VC_HANDLE_SEMAPHORE,
    VC_HANDLE_IMAGE,
    VC_HANDLE_BUFFER,
    VC_HANDLE_DESCRIPTOR_SET_LAYOUT,
    VC_HANDLE_DESCRIPTOR_SET,
    VC_HANDLE_RENDER_PASS,
    VC_HANDLE_FRAMEBUFFER,
    VC_HANDLE_TYPE_COUNT
} vc_handle_type;

// Anonymous handle
VC_DEF_HANDLE(vc_handle);

VC_DEF_HANDLE(vc_compute_pipe);
VC_DEF_HANDLE(vc_semaphore);
VC_DEF_HANDLE(vc_command_buffer);
VC_DEF_HANDLE(vc_image);
VC_DEF_HANDLE(vc_buffer);
VC_DEF_HANDLE(vc_descriptor_set_layout);
VC_DEF_HANDLE(vc_descriptor_set);
VC_DEF_HANDLE(vc_render_pass);
VC_DEF_HANDLE(vc_framebuffer);

// Unpacks the two 32 bits part of the handle
typedef union
{
    vc_handle    packed;

    struct
    {
        handle            hndl;
        vc_handle_type    type;
    };
} vc_handle_unpack;

// A null handle
#define VC_NULL_HANDLE 0L

// Associates an u64 to each managed type, used to store the size of managed type, or max count
typedef u64 vc_handle_mgr_counts[VC_HANDLE_TYPE_COUNT];

// Function that is executed on each object upon destroying handle mgr
typedef b8 (*vc_man_destroy_func)(void *ctx, void *managed_object);

// The handle manager struct
typedef struct
{
    handle_mgr              handle_managers[VC_HANDLE_TYPE_COUNT];
    vc_man_destroy_func     destroy_funcs[VC_HANDLE_TYPE_COUNT];
    vc_handle              *destroy_queue; // darray
    vc_handle_mgr_counts    sizes;
    vc_handle_mgr_counts    counts;
} vc_handle_mgr;

/**
 * @brief Creates a vc handle manager
 *
 * @param[out] mgr The manager
 * @param counts The number of handle allowed for each type
 * @return b8 Success ?
 */
b8            vc_handle_mgr_create(vc_handle_mgr *mgr, vc_handle_mgr_counts counts);

/**
 * @brief Sets up a destroy function for a specific handle type
 *
 * @param mgr
 * @param type The type of handle on which to set the destroy fucntion
 * @param func The destroy function
 */
void          vc_handle_mgr_set_destroy_func(vc_handle_mgr *mgr, vc_handle_type type, vc_man_destroy_func func);

/**
 * @brief Allocates a handle
 *
 * @param mgr
 * @param type The type to allocate
 * @return vc_handle The handle of the new allocation
 */
vc_handle     vc_handle_mgr_alloc(vc_handle_mgr *mgr, vc_handle_type type);

/**
 * @brief Write data in allocation
 *
 * @param mgr
 * @param hndl The handle in which to write data
 * @param object A pointer to the object to read
 */
void          vc_handle_mgr_write(vc_handle_mgr *mgr, vc_handle hndl, void *object);

/**
 * @brief Allocates and write object in a new allocation
 *
 * @param mgr
 * @param type The type to allocate
 * @param object A pointer to the object to read
 * @return vc_handle The handle of the new allocation
 */
vc_handle     vc_handle_mgr_write_alloc(vc_handle_mgr *mgr, vc_handle_type type, void *object);

/**
 * @brief Gets a pointer to the allocation
 *
 * @param mgr
 * @param hndl Handle
 * @return void* Pointer to the allocation (can be modified)
 */
void         *vc_handle_mgr_ptr(vc_handle_mgr *mgr, vc_handle hndl);

/**
 * @brief Frees a handle from the system
 *
 * @param mgr
 * @param hndl The handle to free
 * @param destroy_ctx The context pointer to give to the destroy function upon destroying
 * @return b8 Success
 */
b8            vc_handle_mgr_free(vc_handle_mgr *mgr, vc_handle hndl, void *destroy_ctx);

/**
 * @brief Destroys the handle manager, as well as all the allocated object (by calling their respective destroying functions)
 *
 * @param mgr
 * @param destroy_ctx The context pointer to give to the destroy functions upon destroying
 */
void          vc_handle_mgr_destroy(vc_handle_mgr *mgr, void *destroy_ctx);

/**
 * @brief Converts a handle type to string
 *
 * @param type
 * @return const char* The string
 */
const char   *vc_handle_type_to_str(vc_handle_type    type);