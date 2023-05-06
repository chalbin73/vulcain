#pragma once

#include "../types.h"
#include "../memory.h"
#include "../math.h"

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
#define DARRAY_RESIZE_FACTOR 2
#define DARRAY_MAGIC_NUMBER 0xFEEDBEEF


//Private functions
//NOTE: Those should not be called directly outside the darray implementation
void *_darray_create(u64 capacity, u64 stride, enum memory_alloc_tag);
void _darray_destroy(void *ptr);

void *_darray_resize(void *array, u64 new_size);

void *_darray_push(void *array, void *data);
void *_darray_pop(void *array, void *dest);

void *_darray_insert_at(void *array, u64 index, void *obj);
void *_darray_pop_at(void *array, u64 index, void *dest);

//u64 _darray_index_of(void *array, void *elem);
u64 _darray_get_field(void *array, u64 field);
void _darray_set_field(void *array, u64 field, u64 value);

//"public functions"
// NOTE: No oo_* prefix to shorten the name length, knowing that the darray will be used alot
#define darray_create(type) \
    _darray_create(DARRAY_DEFAULT_CAPACITY, sizeof(type), MEMORY_TAG_DARRAY)

#define darray_create_tag(type, tag) \
    _darray_create(DARRAY_DEFAULT_CAPACITY, sizeof(type), tag)

#define darray_destroy(array) \
    _darray_destroy(array)

#define darray_length(array) \
    _darray_get_field(array, DARRAY_LENGTH)

#define darray_capacity(array) \
    _darray_get_field(array, DARRAY_CAPACITY)

#define darray_stride(array) \
    _darray_get_field(array, DARRAY_STRIDE)

#define darray_tag(array) \
    _darray_get_field(array, DARRAY_TAG)

#define darray_reserve(type, capacity) \
    _darray_create(capacity, sizeof(type))

#define darray_push(array, obj)                  \
    {                                            \
        typeof(obj) temp = obj;                  \
        array = _darray_push(array, &temp);  \
    }

#define darray_pop(array, dest) \
    array = _darray_pop(array, dest)

#define darray_insert_at(array, obj, index)                  \
    {                                                        \
        typeof(obj) temp = obj;                              \
        array = _darray_insert_at(array, index, &temp);  \
    }

#define darray_pop_at(array, index, dest) \
    array = _darray_pop_at(array, index, dest)
