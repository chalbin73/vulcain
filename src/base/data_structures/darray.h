/**
 * @file darray.h
 * @brief This file along with darray.c contains the code for a generic, dynamic array implementation
 */

#ifndef __BASE_DARRAY__
#define __BASE_DARRAY__

#include "../math.h"
#include "../memory.h"
#include "../types.h"

enum darray_fields
{
    DARRAY_CAPACITY,
    DARRAY_LENGTH,
    DARRAY_STRIDE,
    DARRAY_TAG,
    DARRAY_FIELD_LENGTH
};

/*
 * Represents a comparison between two arbitrary objects.
 * result r is :
 *  - r < 0 if a < b
 *  - r > 0 if a > b
 *  - r = 0 if a = b
 */
typedef i32 (*darray_usr_compare_func)(void *a, void *b);

#define DARRAY_DEFAULT_CAPACITY 1
#define DARRAY_RESIZE_FACTOR    2
#define DARRAY_MAGIC_NUMBER     0xFEEDBEEF

/*
 * @brief Syntaxic sugar for more verbose communication of type.
 */
#define darray(type) \
        type *

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

b8      _darray_is_sorted(void *array, darray_usr_compare_func cmp);
b8      _darray_is_strictly_sorted(void *array, darray_usr_compare_func cmp);
b8      _darray_qsort(void *array, darray_usr_compare_func cmp);

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
 * @attention This macro-function may modify the array pointer.
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
 * @attention This macro-function may modify the array pointer.
 */
#define darray_push_ptr(array, ptr)       \
        {                                     \
            array = _darray_push(array, ptr); \
        }

/**
 * @brief Pops an object: removes and returns the last object of the array
 *
 * @param array The array
 * @attention This macro-function may modify the array pointer.
 */
#define darray_pop(array, dest) \
        array = _darray_pop(array, dest);

/**
 * @brief Inserts an object in the array at a specific index
 *
 * @param array The array
 * @param obj The object to insert
 * @param index The index of the added object
 *
 * @note It is garanteed that doing `array[index]` returns the same object inserted with this function
 * @attention This will not work with string literals
 * @attention This macro-function may modify the array pointer.
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
 * @attention This macro-function may modify the array pointer.
 */
#define darray_pop_at(array, index, dest) \
        array = _darray_pop_at(array, index, dest);

/**
 * @brief Sorts the array, based on a user provided comparison function (order on a set)
 *
 * @param array The array
 * @param cmp The comparison function
 *
 * @note This sort uses the quick sort algorithm. Worst cas is O(n^2). Best case/average is O(n log n).
 * @note It must be guaranteed that darray_sorted returns true after this call. The list may not necessarly strictly sorted.
 * @attention This function modifies memory, but does not require any reallocation/pointer modification.
 */
#define darray_qsort(array, cmp) \
        _darray_qsort(array, (darray_usr_compare_func)cmp);

/**
 * @brief Checks wether or not the array is sorted, using a totally ordered set relation, user provided.
 *
 * @param array The array
 * @param cmp The comparison function
 *
 * @note Two elements a,b are considered as sorted if cmp(a, b) = 0.
 */
#define darray_sorted(array, cmp) \
        _darray_is_sorted(array, (darray_usr_compare_func)cmp)

/**
 * @brief Checks wether or not the array is strictly sorted, using a totally ordered set relation, user provided.
 *
 * @param array The array
 * @param cmp The comparison function
 *
 * @note Two elements a,b are not considered as sorted if cmp(a, b) = 0.
 */
#define darray_strict_sorted(array, cmp) \
        _darray_is_strictly_sorted(array, (darray_usr_compare_func)cmp)

#endif //__BASE_DARRAY__

