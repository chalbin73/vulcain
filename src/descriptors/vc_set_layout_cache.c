#include "vc_set_layout_cache.h"

#include "../base/data_structures/darray.h"
#include "../vc_enum_util.h"

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
vc_slc_create(vc_set_layout_cache   *cache)
{
    for(u32 i = 0; i < VC_SLC_BUCKET_COUNT; i++)
    {
        cache->buckets[i] = darray_create(_vc_slc_cell);
    }
}

u64
_vc_slc_hash(VkDescriptorSetLayoutCreateInfo   *d)
{
    u64 result = d->bindingCount * d->flags;

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
    u64 hash         = _vc_slc_hash(&info);
    u64 index        = hash % VC_SLC_BUCKET_COUNT;
    u32 bucket_count = darray_length(cache->buckets[index]);

    for(u32 i = 0; i < bucket_count; i++)
    {
        if(cache->buckets[index][i].hash == hash)
        {
            return cache->buckets[index][i].layout;
        }
    }

    return VK_NULL_HANDLE;
}

VkDescriptorSetLayout
vc_slc_get(vc_set_layout_cache *cache, VkDevice dev, VkDescriptorSetLayoutCreateInfo info)
{
    VkDescriptorSetLayoutBinding *bindings = darray_create(VkDescriptorSetLayoutBinding);
    for(u32 i = 0; i < info.bindingCount; i++)
    {
        darray_push(bindings, info.pBindings[i]);
    }
    darray_qsort(bindings, _vc_slc_binding_comp);
    info.pBindings = bindings;

    // Search if set layout already extists in structure
    VkDescriptorSetLayout layout = _vc_slc_lookup(cache, info);

    if(layout == VK_NULL_HANDLE)
    {
        // Layout does not exist in the structure yet. Create it
        VK_CHECK(vkCreateDescriptorSetLayout(dev, &info, NULL, &layout), "Could not create a descriptor set layout.");

        // Add layout to structure.
        u64 hash              = _vc_slc_hash(&info);
        _vc_slc_cell new_cell =
        {
            .layout = layout,
            .hash   = hash,
        };
        darray_push(cache->buckets[hash % VC_SLC_BUCKET_COUNT], new_cell);
    }

    return layout;
}

void
vc_slc_destroy(vc_set_layout_cache *cache, VkDevice dev)
{
    for(u32 i = 0; i < VC_SLC_BUCKET_COUNT; i++)
    {
        for(u32 j = 0; j < darray_length(cache->buckets[i]); j++)
        {
            vkDestroyDescriptorSetLayout(dev, cache->buckets[i][j].layout, NULL);
        }
        darray_destroy(cache->buckets[i]);
    }
}

