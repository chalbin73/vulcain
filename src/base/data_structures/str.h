#pragma once

/**
 * @file str.h
 * @author Albin Chaboissier (chalbin73@gmail.com)
 * @brief A simple custom string data structure
 * @version 0.1
 * @date 2023-05-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "../base.h"

typedef struct
{
    u64    str_length;
    u8    *chars;
} str;

void    str_create(str *string, u8 *data, u64 data_length);

#define str_from_literal(str, lit) \
    str_create(str, lit, sizeof(lit) - 1);

void    str_free(str   *string);

