#include "darray.h"

void *
_darray_create(u64 capacity, u64 stride, enum memory_alloc_tag tag)
{
    u64 header_size = DARRAY_FIELD_LENGTH * sizeof(u64);
    u64 array_size  = capacity * stride;
    u64 *new_array  = mem_allocate(array_size + header_size, tag);
    mem_memset(new_array, 0, array_size + header_size);
    new_array[DARRAY_CAPACITY] = capacity;
    new_array[DARRAY_LENGTH]   = 0;
    new_array[DARRAY_STRIDE]   = stride;
    new_array[DARRAY_TAG]      = tag;
    return (void *)(new_array + DARRAY_FIELD_LENGTH);
}

void
_darray_destroy(void   *ptr)
{
    if (ptr)
    {
        u64 *header = (u64 *)ptr - DARRAY_FIELD_LENGTH;
        header[DARRAY_CAPACITY] = 0;
        header[DARRAY_STRIDE]   = 0;
        mem_free(header);
    }
}

u64
_darray_get_field(void *array, u64 field)
{
    u64 *header = (u64 *)array - DARRAY_FIELD_LENGTH;
    return header[field];
}

void *
_darray_resize(void *array, u64 new_size)
{
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    u64 tag    = darray_tag(array);

    void *temp = _darray_create(new_size, stride, tag);
    mem_memcpy(temp, array, length * stride);

    _darray_set_field(array, DARRAY_LENGTH, length);
    _darray_destroy(array);
    return temp;
}

void
_darray_set_field(void *array, u64 field, u64 value)
{
    u64 *header = (u64 *)array - DARRAY_FIELD_LENGTH;
    header[field] = value;
}

void *
_darray_push(void *array, void *data)
{
    u64 length   = darray_length(array);
    u64 stride   = darray_stride(array);
    u64 capacity = darray_capacity(array);

    if ( length >= darray_capacity(array) )
    {
        array = _darray_resize(array, capacity * DARRAY_RESIZE_FACTOR);
    }

    u64 addr = (u64)array;
    addr += (length * stride);
    mem_memcpy( (void *)addr, data, stride );
    _darray_set_field(array, DARRAY_LENGTH, length + 1);
    return array;
}

void *
_darray_pop(void *array, void *dest)
{
    u64 length   = darray_length(array);
    u64 stride   = darray_stride(array);
    u64 capacity = darray_capacity(array);

    u64 addr = (u64)array;
    addr += (length - 1) * stride;
    mem_memcpy(dest, (void *)addr, stride);
    _darray_set_field(array, DARRAY_LENGTH, length - 1);
    (void)capacity; // TODO: Allow darray to shrink<
    return array;
}

void *
_darray_pop_at(void *array, u64 index, void *dest)
{
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);

    if(index >= length)
    {
        ERROR("Index outside of bound of darray, length=%d index=%d", length, index);
        return array;
    }

    u64 addr = (u64)array;
    if (dest != NULL)
    {
        mem_memcpy(dest, (void *)( addr + (index * stride) ), stride);
    }

    if ( index != (length - 1) )
    {
        mem_memmove(
            (void *)( addr + (index * stride) ),
            (void *)( addr + ( (index + 1) * stride ) ),
            stride * ( (length - index) - 1 )
            );
    }

    _darray_set_field(array, DARRAY_LENGTH, length - 1);
    return array;
}

void *
_darray_insert_at(void *array, u64 index, void *obj)
{

    u64 length   = darray_length(array);
    u64 stride   = darray_stride(array);
    u64 capacity = darray_capacity(array);

    if (index >= length)
    {
        ERROR("Index outside of bound of darray, length=%d index=%d", length, index);
        return array;
    }

    if ( length >= darray_capacity(array) )
    {
        array = _darray_resize(array, capacity * DARRAY_RESIZE_FACTOR);
    }

    u64 addr = (u64)array;

    if (index != length - 1)
    {
        mem_memmove(
            (void *)( addr + ( (index + 1) * stride ) ),
            (void *)( addr + (index * stride) ),
            stride * (length - index)
            );
    }

    mem_memcpy( (void *)( addr + (index * stride) ), obj, stride );

    _darray_set_field(array, DARRAY_LENGTH, length + 1);
    return array;
}

// Sorting functions
b8
_darray_is_sorted(void *array, darray_usr_compare_func cmp)
{
    u64 length = _darray_get_field(array, DARRAY_LENGTH);
    u64 stride = _darray_get_field(array, DARRAY_STRIDE);

    void *prev = array;
    for(u64 i = 1; i < length; i++)
    {
        void *new = (void *)( (u64)prev + stride );

        if(cmp(prev, new) > 0)
        {
            return FALSE;
        }
        prev = new;
    }

    return TRUE;
}

b8
_darray_is_strictly_sorted(void *array, darray_usr_compare_func cmp)
{
    u64 length = _darray_get_field(array, DARRAY_LENGTH);
    u64 stride = _darray_get_field(array, DARRAY_STRIDE);

    void *prev = array;
    for(u64 i = 1; i < length; i++)
    {
        void *new = (void *)( (u64)prev + stride );

        if(cmp(prev, new) >= 0)
        {
            return FALSE;
        }
        prev = new;
    }

    return TRUE;
}

// Convenience function, to do the necessary casts
#define _DARRAY_INDEX(array, stride, i) \
        ( (void *)( (u64)(array) + ( (i) * (stride) ) ) )

void
_darray_qsort_vswap(void *a, void *b, u64 byte_count)
{
    // Compilers should be able to optimize this code quite well
    u8 *p;
    u8 *q;
    u8 *end = (u8 *)( (u64)a + byte_count );

    u8 t;
    for(p = a, q = b; p < end; p++, q++)
    {
        t  = *p;
        *p = *q;
        *q = t;
    }
}

#include <alloca.h>
#define _DARRAY_SWAP(array, stride, a, b)               \
        if(a != b) \
        {                                               \
            void *a_p = _DARRAY_INDEX(array, stride, a); \
            void *b_p = _DARRAY_INDEX(array, stride, b); \
            _darray_qsort_vswap(a_p, b_p, stride);   \
        }

// Quick sort implementation
u64
_darray_qsort_partition(void *array, u64 stride, u64 start, u64 end, darray_usr_compare_func cmp)
{
    // Chose pivot
    u64 pivot_index = ( (end - 1L) + start ) / 2L;
    //u64 pivot_index = end - 1L;

    // Move pivot to end
    _DARRAY_SWAP(array, stride, pivot_index, end - 1L);
    void *pivot = _DARRAY_INDEX(array, stride, end - 1L);

    // Start of elements greater than pivot
    u64 i_sup = start;

    for(u64 i = start; i < end - 1L; i++)
    {
        // Compare current and pivot
        void *current = _DARRAY_INDEX(array, stride, i);
        i32 cmp_r     = cmp(current, pivot);

        if(cmp_r < 0) // current smaller that pivot
        {
            _DARRAY_SWAP(array, stride, i_sup, i);
            i_sup++;
        }
        else // current bigger that pivot
        {
            // NO-OP
        }
    }

    // Partition finished. Move pivot in between two zones
    _DARRAY_SWAP(array, stride, i_sup, end - 1L);

    // Return new pivot position
    return i_sup;
}

void
_darray_qsort_rec(void *array, u64 stride, u64 start, u64 end, darray_usr_compare_func cmp)
{
    if(start < end )
    {
        //printf("%ud -- %ud\n", start, end);
        u64 pivot_index = _darray_qsort_partition(array, stride, start, end, cmp);

        // Quick sort the two partitioned areas
        _darray_qsort_rec(array, stride, start, pivot_index, cmp);
        _darray_qsort_rec(array, stride, pivot_index + 1L, end, cmp);
    }
}

b8
_darray_qsort(void *array, darray_usr_compare_func cmp)
{
    u64 length = _darray_get_field(array, DARRAY_LENGTH);
    u64 stride = _darray_get_field(array, DARRAY_STRIDE);

    _darray_qsort_rec(array, stride, 0L, length, cmp);
    return TRUE;
}

#undef _DARRAY_INDEX
#undef _DARRAY_SWAP

