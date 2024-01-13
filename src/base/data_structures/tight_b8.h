#pragma once

/**
 * @file tight_b8.h
 * @author Albin Chaboisser (chalbin73@gmail.com)
 * @brief A tight boolean implementation (allows to store 8 booleans par bytes)
 * @version 0.1
 * @date 2023-05-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "../memory.h"
#include "../types.h"

#define TB8_MOD8(x)     ( (x) & 7 )
#define TB8_U64_DIV8(x) ( (x) >> 3 )

/**
 * @brief Sets all the boolean to false in a tight b8
 *
 * @param array The tight boolean array
 * @param bool_count The number of booleans
 * @return b8 TRUE
 */
b8    tb8_set_all_false(b8 *array, u64 bool_count);

/**
 * @brief Sets all the boolean to true in a tight b8
 *
 * @param array The tight boolean array
 * @param bool_count The number of booleans
 * @return b8 TRUE
 */
b8    tb8_set_all_true(b8 *array, u64 bool_count);

/**
 * @brief Sets a boolean in a tight b7
 *
 * @param array The tight boolean array
 * @param index The index of the boolean
 * @param value The value to set the index to
 * @return b8 TRUE
 */
b8    tb8_set(b8 *array, u64 index, b8 value);

/**
 * @brief Toggles a boolean in the array
 *
 * @param array The tight boolean array
 * @param index The index to toggle
 * @return b8 The value of the boolean before the toggle
 */
b8    tb8_toggle(b8 *array, u64 index);

/**
 * @brief Gets a boolean value in the array
 *
 * @param array The tight boolean array
 * @param index The index to get
 * @return b8 The value at index
 */
b8    tb8_get(b8 *array, u64 index);

/**
 * @brief Wether or not the array contains a true
 *
 * @param array The tight boolean array
 * @param bool_count The number of booleans
 * @return b8 TRUE or FALSE
 */
b8    tb8_some_true(b8 *array, u64 bool_count);

/**
 * @brief Wether or not all the boolean are false
 *
 * @param array The tight boolean array
 * @param bool_count The number of booleans
 * @return b8 TRUE or FALSe
 */
b8    tb8_all_true(b8 *array, u64 bool_count);

