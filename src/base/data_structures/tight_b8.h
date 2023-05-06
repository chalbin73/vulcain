#pragma once

// This translation unit contains functions to deal with a tightly packed boolean array
// that is, instead of holding on boolean per 8-bit, we hold 1 boolean per bit in an array of bytes

#include "../memory.h"
#include "../types.h"

b8 tb8_set_all_false(b8 *array, u64 bool_count);
b8 tb8_set_all_true(b8 *array, u64 bool_count);

b8 tb8_set(b8 *array, u64 index, b8 value);
b8 tb8_toggle(b8 *array, u64 index);

b8 tb8_get(b8 *array, u64 index);

b8 tb8_some_true(b8 *array, u64 bool_count);
b8 tb8_all_true(b8 *array, u64 bool_count);
