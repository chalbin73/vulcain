#include "hashmap.h"

void    hashmap_create(hashmap *hm, u32 bucket_count, u64 key_stride, u64 obj_stride,
                       hashmap_hash_func hash_func)

{
    hm->bucket_count = bucket_count;
    hm->key_stride   = key_stride;
    hm->obj_stride   = obj_stride;
    hm->hash_func    = hash_func;
    hm->count        = 0;

    hm->buckets = mem_allocate(sizeof(void *) * bucket_count, MEMORY_TAG_RENDERER);
    for (int i = 0; i < bucket_count; i++)
    {
        hm->buckets[i] = darray_create_sized(key_stride + obj_stride);
    }
}

void    hashmap_destroy(hashmap   *hm)
{
    for (int i = 0; i < hm->bucket_count; i++)
    {
        darray_destroy(hm->buckets[i]);
    }
    mem_free(hm->buckets);
}

void    hashmap_insert(hashmap *hm, void *key, void *object)
{
    u64 bidx = hm->hash_func(key, hm->key_stride) % hm->bucket_count;

    void *dat = alloca(hm->key_stride + hm->obj_stride);
    mem_memcpy(dat, key, hm->key_stride);
    mem_memcpy( (void *)( (u64)dat + hm->key_stride ), object, hm->obj_stride );

    // TODO: Handle already inserted objects
    darray_push_ptr(hm->buckets[bidx], dat);
    hm->count++;
}

void   *hashmap_lookup(hashmap *hm, void *key)
{
    u64 bidx   = hm->hash_func(key, hm->key_stride) % hm->bucket_count;
    void *buck = hm->buckets[bidx];
    void *elem = buck;

    u32 count = darray_length(buck);
    for (int i = 0; i < count; i++)
    {
        if (mem_memcmp(elem, key, hm->key_stride) == 0)
        {
            return (void *)( (u64)elem + hm->key_stride );
        }
        elem = (void *)( (u64)elem + (hm->key_stride + hm->obj_stride) );
    }

    // Not found
    return NULL;
}

b8    hashmap_remove(hashmap *hm, void *key)
{
    u64 bidx   = hm->hash_func(key, hm->key_stride) % hm->bucket_count;
    void *buck = hm->buckets[bidx];
    void *elem = buck;

    b8 removed = FALSE;
    for (int i = 0; i < darray_length(buck); i++)
    {
        if (mem_memcmp(elem, key, hm->key_stride) == 0)
        {
            darray_pop_at(buck, i, NULL);
            removed = TRUE;
            hm->count--;
            return TRUE;
        }
        elem = (void *)( (u64)elem + hm->key_stride + hm->obj_stride );
    }

    return removed;
}

