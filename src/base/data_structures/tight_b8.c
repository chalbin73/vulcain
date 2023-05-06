#include "tight_b8.h"

b8 tb8_set_all_false(b8 *array, u64 bool_count)
{
    mem_memset(array, 0x00, (bool_count / 8) + (bool_count % 8 == 0 ? 0 : 1));
    return TRUE;
}

b8 tb8_set_all_true(b8 *array, u64 bool_count)
{
    mem_memset(array, 0xFF, (bool_count / 8) + (bool_count % 8 == 0 ? 0 : 1));
    return TRUE;
}

b8 tb8_set(b8 *array, u64 index, b8 value)
{
    // Get the byte
    u8 *byte = (u8 *)&array[index / 8];

    // Set the value within the byte
    if (value)
    {
        *byte |= 1 << (index % 8);
    }
    else
    {
        *byte &= ~(1 << (index % 8));
    }
    return TRUE;
}

b8 tb8_toggle(b8 *array, u64 index)
{
    b8 val = tb8_get(array, index);
    tb8_set(array, index, !val);
    return val;
}

b8 tb8_get(b8 *array, u64 index)
{
    // Get the byte
    u8 *byte = (u8 *)&array[index / 8];

    // Get the value within the byte

    return ((*byte >> (index % 8)) & 1) != 0;
}

b8 tb8_some_true(b8 *array, u64 bool_count)
{
    u64 byte_count = (bool_count / 8) + (bool_count % 8 == 0 ? 0 : 1);

    // Browse every byte
    for (int i = 0; i < byte_count; i++)
    {
        if (array[i])
        {
            return TRUE;
        }
    }

    return FALSE;
}

b8 tb8_all_true(b8 *array, u64 bool_count)
{
    u64 byte_count = (bool_count / 8) + (bool_count % 8 == 0 ? 0 : 1);

    // Browse every byte
    for (int i = 0; i < byte_count; i++)
    {
        if (array[i] != ((b8)0xFF))
        {
            return FALSE;
        }
    }

    return TRUE;
}