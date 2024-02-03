#include "vc_set_layout_cache.h"

#include "../base/data_structures/darray.h"

i32
_vc_slc_binding_comp(VkDescriptorSetLayoutBinding *a, VkDescriptorSetLayoutBinding *b)
{
    return a->binding - b->binding;
}

// Compares two SORTED set layout infos
b8
_vc_slc_info_comp(VkDescriptorSetLayoutCreateInfo a, VkDescriptorSetLayoutCreateInfo b)
{
    if(
        a.flags != b.flags ||
        a.bindingCount != b.bindingCount
        )
    {
        return FALSE;
    }

    // Compare each bindings
    for(u32 i = 0; i < a.bindingCount; i++)
    {
        VkDescriptorSetLayoutBinding a_b = a.pBindings[i];
        VkDescriptorSetLayoutBinding b_b = b.pBindings[i];

        // Since both lists are supposed to be sorted, we can compare both lists by going through them
        if(
            a_b.binding != b_b.binding ||
            a_b.descriptorType != b_b.descriptorType ||
            a_b.stageFlags != b_b.stageFlags ||
            a_b.descriptorCount != b_b.descriptorCount
            )
        {
            return FALSE;
        }
    }

    return TRUE;
}

void
vc_slc_create(vc_set_layout_cache *cache, u32 set_layout_pool_size, u32 binding_pool_size)
{
    mem_pool_create(
        &cache->set_layouts_pool,
        mem_allocate(sizeof(_vc_slc_cell) * set_layout_pool_size, MEMORY_TAG_RENDERER),
        sizeof(_vc_slc_cell),
        set_layout_pool_size
        );

    mem_pool_create(
        &cache->bindings_pool,
        mem_allocate(sizeof(_vc_slc_binding_cell) * binding_pool_size, MEMORY_TAG_RENDERER),
        sizeof(_vc_slc_binding_cell),
        binding_pool_size
        );

    for(u32 i = 0; i < VC_SLC_BUCKET_COUNT; i++)
    {
        cache->buckets[i] = NULL;
    }
}

u64
_vc_slc_hash(VkDescriptorSetLayoutCreateInfo   *d)
{
    u64 result = d->bindingCount;

    for (int i = 0; i < d->bindingCount; i++)
    {
        VkDescriptorSetLayoutBinding b = d->pBindings[i];
        u64 binding_hash               = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

        result ^= binding_hash;
    }

    return result;
}

VkDescriptorSetLayout
_vc_slc_lookup(vc_set_layout_cache *cache, VkDescriptorSetLayoutCreateInfo info)
{
    _vc_slc_cell hash = _vc_slc_hash(&info);
}

VkDescriptorSetLayout
vc_slc_get(vc_set_layout_cache *cache, VkDescriptorSetLayoutCreateInfo info)
{
    VkDescriptorSetLayoutBinding *bindings = darray_create(VkDescriptorSetLayoutBinding);
    for(u32 i = 0; i < info.bindingCount; i++)
    {
        darray_push(bindings, info.pBindings[i]);
    }
    darray_qsort(bindings, _vc_slc_binding_comp);

    info.pBindings = bindings;

    // Search if set layout already extists in structure

}

