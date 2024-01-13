#pragma once

#include "../math.h"
#include "../memory.h"
#include "../types.h"

// NOTE: Epideon implementation of dynamic array
enum darray_fields
{
    DARRAY_CAPACITY,
    DARRAY_LENGTH,
    DARRAY_STRIDE,
    DARRAY_TAG,
    DARRAY_FIELD_LENGTH
};

#define DARRAY_DEFAULT_CAPACITY 1
#define DARRAY_RESIZE_FACTOR    2
#define DARRAY_MAGIC_NUMBER     0xFEEDBEEF

// Private functions
// NOTE: Those should not be called directly outside the darray implementation
void   *_darray_create(u64 capacity, u64 stride, enum memory_alloc_tag);
void    _darray_destroy(void   *ptr);

void   *_darray_resize(void *array, u64 new_size);

void   *_darray_push(void *array, void *data);
void   *_darray_pop(void *array, void *dest);

void   *_darray_insert_at(void *array, u64 index, void *obj);
void   *_darray_pop_at(void *array, u64 index, void *dest);

u64     _darray_get_field(void *array, u64 field);
void    _darray_set_field(void *array, u64 field, u64 value);

/**
 * @brief Creates a dynamic array, with the typename provided.
 *
 * @param type The typename of the stored object
 */
#define darray_create(type) \
    _darray_create(DARRAY_DEFAULT_CAPACITY, sizeof(type), MEMORY_TAG_DARRAY)

/**
 * @brief Creates a dynamic array, with the size provided.
 *
 * @param type The size of the stored object
 */
#define darray_create_sized(size) \
    _darray_create(DARRAY_DEFAULT_CAPACITY, size, MEMORY_TAG_DARRAY)

/**
 * @brief Creates a dynamic array, with the typename and memory tag provided.
 *
 * @param type The typename of the stored object
 * @param tag The memory tag to use
 */
#define darray_create_tag(type, tag) \
    _darray_create(DARRAY_DEFAULT_CAPACITY, sizeof(type), tag)

/**
 * @brief Creates a dynamic array, with the typename and the capacity provided.
 *
 * @param type The typename of the stored object
 * @param capacity The capacity to reserve
 */
#define darray_reserve(type, capacity) \
    _darray_create( capacity, sizeof(type) )

/**
 * @brief Destroys an dynamic array
 *
 * @param array The array
 */
#define darray_destroy(array) \
    _darray_destroy(array)

/**
 * @brief Gets the number of elements in the array
 *
 * @param array The array
 */
#define darray_length(array) \
    _darray_get_field(array, DARRAY_LENGTH)

/**
 * @brief Gets the current capacity of the array ()
 *
 * @param array The array
 */
#define darray_capacity(array) \
    _darray_get_field(array, DARRAY_CAPACITY)

/**
 * @brief Gets the stride of the array (the size of a stored object) in bytes
 *
 * @param array The array
 */
#define darray_stride(array) \
    _darray_get_field(array, DARRAY_STRIDE)

/**
 * @brief Gets the memory tag used for allocations
 *
 * @param array The array
 */
#define darray_tag(array) \
    _darray_get_field(array, DARRAY_TAG)

/**
 * @brief Pushes an object in the array
 *
 * @param array The array
 * @param obj The object to copy and push into the array
 * @attention This will not work with string literals
 */
#define darray_push(array, obj)             \
    {                                       \
        typeof(obj) temp = obj;             \
        array = _darray_push(array, &temp); \
    }

/**
 * @brief Pushes an object pointed to in the array
 *
 * @param array The array
 * @param ptr A pointer from which to copy from the data to store
 * @attention This will not work with string literals
 */
#define darray_push_ptr(array, ptr)       \
    {                                     \
        array = _darray_push(array, ptr); \
    }

/**
 * @brief Pops an object: removes and returns the last object of the array
 *
 * @param array The array
 */
#define darray_pop(array, dest) \
    array = _darray_pop(array, dest)

/**
 * @brief Inserts an object in the array at a specific index
 *
 * @param array The array
 * @param obj The object to insert
 * @param index The index of the added object
 *
 * @note It is garanteed that doing `array[index]` returns the same object inserted with this function
 * @attention This will not work with string literals
 */
#define darray_insert_at(array, obj, index)             \
    {                                                   \
        typeof(obj) temp = obj;                         \
        array = _darray_insert_at(array, index, &temp); \
    }
/**
 * @brief Pops an object from an index: removes and returns an object at a specific index
 *
 * @param array The array
 * @param index The index of the object to remove
 * @param dest The destination of the removed object
 *
 * @note It is garanteed that doing `array[index]` returns the object that was after the removed object.
 */
#define darray_pop_at(array, index, dest) \
    array = _darray_pop_at(array, index, dest)

