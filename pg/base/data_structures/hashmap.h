/**
 * @file      hashmap.h
 * @brief     Header of a simple hashmap implementation
 * @date      Sat May 20 19:36:17 2023
 * @author    albin
 * @copyright BSD-3-Clause
 *
 * This module
 */

#pragma once

#include "../base.h"
#include "darray.h"
#include <alloca.h>

typedef u64 (*hashmap_hash_func)(void *obj, u64 size);

typedef struct
{
    u32                  bucket_count;
    void               **buckets; // darrays
    hashmap_hash_func    hash_func;

    u64                  obj_stride;
    u64                  key_stride;

    u64                  count;
} hashmap;

void    hashmap_create(hashmap *hm, u32 bucket_count, u64 key_stride, u64 obj_stride,
                       hashmap_hash_func hash_func);

void    hashmap_destroy(hashmap   *hm);

void    hashmap_insert(hashmap *hm, void *key, void *object);
void   *hashmap_lookup(hashmap *hm, void *key);
b8      hashmap_remove(hashmap *hm, void *key);

#define hashmap_count(hm) \
    ( (hm)->count )

