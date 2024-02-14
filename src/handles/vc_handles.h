#ifndef __VC_HANDLES__
#define __VC_HANDLES__

#include "../base/base.h"
#include "vc_handle_pool.h" // TODO: Figure out a neat way to not include this

typedef enum
{
    VC_HANDLE_SWAPCHAIN = 0,
    VC_HANDLE_QUEUE,
    VC_HANDLE_COMMAND_POOL,
    VC_HANDLE_COMMAND_BUFFER,
    VC_HANDLE_SEMAPHORE,
    VC_HANDLE_IMAGE,
    VC_HANDLE_IMAGE_VIEW,
    VC_HANDLE_COMPUTE_PIPELINE,
    VC_HANDLE_GFX_PIPELINE,
    VC_HANDLE_DESCRIPTOR_SET,
    VC_HANDLE_DESCRIPTOR_SET_LAYOUT,
    VC_HANDLE_BUFFER,
    VC_HANDLE_TYPES_COUNT,
} vc_handle_type;

typedef u64                  vc_handle;

#define VC_DEF_HANDLE(hndl) \
        typedef vc_handle    hndl;

#define VC_NULL_HANDLE (0L) // A handle cannot be 0, as it would mean the underlying vc_handle_pool id is null, which is invalid

VC_DEF_HANDLE(vc_swapchain);
VC_DEF_HANDLE(vc_queue);
VC_DEF_HANDLE(vc_command_pool);
VC_DEF_HANDLE(vc_command_buffer);
VC_DEF_HANDLE(vc_semaphore);
VC_DEF_HANDLE(vc_image);
VC_DEF_HANDLE(vc_image_view);
VC_DEF_HANDLE(vc_compute_pipeline);
VC_DEF_HANDLE(vc_gfx_pipeline);
VC_DEF_HANDLE(vc_descriptor_set);
VC_DEF_HANDLE(vc_descriptor_set_layout);
VC_DEF_HANDLE(vc_buffer);

/*
 * @brief Function pointer for cleanly destroying objects stored in the handle manager
 *
 * @param usr_data The user data
 * @param obj A pointer to the object to be destroyed
 * @param type The type of object to be destroyed
 */
typedef void (*vc_handle_destroy_func)(void *usr_data, void *obj, vc_handle_type type);

typedef struct
{
    vc_handle_pool            pools[VC_HANDLE_TYPES_COUNT];
    vc_handle_destroy_func    destroy_functions[VC_HANDLE_TYPES_COUNT];
    vc_handle                *destroy_queue; // We maintain a destroy queue, to destroy, in order of creation

    void                     *dest_func_usr_data;

} vc_handles_manager;

// Functions

/**
 * @brief Creates a vulcain handle manager
 *
 * @param mgr Pointer to allocated memory region to store mgr into
 */
void      vc_handles_manager_create(vc_handles_manager   *mgr);

/**
 * @brief Destroys handle manager, as well as all objects contained into it with corresponding destroy functions
 *
 * @param mgr The handle manager
 */
void      vc_handles_manager_destroy(vc_handles_manager   *mgr);

/**
 * @brief Dereferences a handle
 *
 * @param mgr The handle manager
 * @param hndl The handle to dereference
 */
void     *vc_handles_manager_deref(vc_handles_manager *mgr, vc_handle hndl);

/**
 * @brief Allocates a handle in the handle manager
 *
 * @param mgr The handle manager
 * @param type The type of handle to manage
 * @return A handle to the new allocated object
 */
vc_handle vc_handles_manager_alloc(vc_handles_manager *mgr, vc_handle_type type);

/**
 * @brief Allocates a handle in the handle manager, and writes the data into the managed object
 *
 * @param mgr The handle manager
 * @param type The type of handle to manage
 * @param obj A pointer to the data to copy into the manager
 * @return A handle to the new allocated object
 */
vc_handle vc_handles_manager_walloc(vc_handles_manager *mgr, vc_handle_type type, void *obj);

/**
 * @brief Deallocates object from manager, without doing anything else
 *
 * @param mgr The manager
 * @param hndl The handle to free
 * @note Calling this function on an handle does not call the corresponding destroy function on the object
 */
void      vc_handles_manager_dealloc(vc_handles_manager *mgr, vc_handle hndl);
/**
 * @brief Destroy a handle in the manager
 *
 * @param mgr The handle manager
 * @param hndl The handle to be destroyed
 * @note Calling this function on a handle calls the corresponding destroy function on the object referenced by the handle
 */
void      vc_handles_manager_destroy_handle(vc_handles_manager *mgr, vc_handle hndl);

/**
 * @brief Sets a destroy function for a particular handle type
 *
 * @param mgr The handle manager
 * @param hndl_type The type of handle for which to register the destroy function
 * @param func The function pointer
 */
void      vc_handles_manager_set_destroy_function(vc_handles_manager *mgr, vc_handle_type hndl_type, vc_handle_destroy_func func);

/**
 * @brief Sets teh user data to be passed to destroy functions
 *
 * @param mgr The handle manager
 * @param usr_data The user data pointer to pass to destroy functions
 */
void      vc_handles_manager_set_destroy_function_usr_data(vc_handles_manager *mgr, void *usr_data);

#endif // __VC_HANDLES__

