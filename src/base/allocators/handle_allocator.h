#pragma once
#include "../base.h"

/**
 * @brief Provides a basic handle manager, for api design. This handle manger handles creation, destruction, memory managment of objects
            hidden behind an opaque handle type.
            The API backend using this handle manager, may use the fucntion interface, and the handle type, in order to operate on the underlying
            objets, and may pass the opaque handles to the user (on the frontend).
            This way, the user, only works with opaque handles, and it secures the use of the api.
            The handle function interface, should not be used bu the user.
 *
 */

/**
 * @brief A handle, this type should preferably not be given, or used, on the frontend of an api, so that handles "types" can be recognised by the user */
typedef u32 handle;
#define NULL_HANDLE 0

typedef struct
{
    u16    unique_counter;
    b8     free;
} handle_mgr_block_header;

/**
 * @brief The actuak handle manager object, functions to operate on this are prefixed with handle_mgr
 *
 */
typedef struct
{
    void   *memory;
    u64     memory_size;

    u64     managed_size;
    u64     block_size;
    u64     alloced_blocks;
    u64     block_count;

    void   *first_free_block;
} handle_mgr;

/**
 * @brief Returns the size of memory to be allocated for use with the handle manager for it to be able to allocate `concurrent_alloc_count` concurrently
 *
 * @param concurrent_alloc_count The number of allocations
 * @param managed_size The size of the object to be managed
 * @return u64 The size of the mermory
 */
u64       handle_mgr_get_size_for_count(u64 concurrent_alloc_count, u64 managed_size);

/**
 * @brief Create the handle memory
 *
 * @param mgr A pointer to a handle manager
 * @param memory Memory that the handle manager manages
 * @param memory_size The size of the memory that the manager will be able to access
 * @param managed_size The size of the object managed by this instance
 * @return b8 Success ?
 */
b8        handle_mgr_create(handle_mgr *mgr, void *memory, u64 memory_size, u64 managed_size);

/**
 * @brief Allocates a new object
 *
 * @param mgr A pointer to a handle manager
 * @return handle A handle to the new free object
 */
handle    handle_mgr_allocate(handle_mgr   *mgr);

/**
 * @brief Frees an object
 *
 * @param mgr A pointer to a handle manager
 * @param hndl The handle of the object ti be freed
 * @return b8 Was a free operation made
 */
b8        handle_mgr_free(handle_mgr *mgr, handle hndl);

/**
 * @brief Dereferences an handle
 *
 * @param mgr A pointer to a handle manager
 * @param hndl The handle to dereference
 * @return void* A pointer to the dereferenced object
 */
void     *handle_mgr_deref(handle_mgr *mgr, handle hndl);

/**
 * @brief Retrieves number of allocated handles
 *
 * @param mgr A pointer to a handle manager
 * @return u64 The number of allocated handles
 */
u64       handle_mgr_get_count(handle_mgr   *mgr);

/**
 * @brief A pointer to a handle manager
 *
 * @param mgr Destroys a handle manager
 * @return void* The memory that was provided to the manager
 */
void     *handle_mgr_destroy(handle_mgr   *mgr);

