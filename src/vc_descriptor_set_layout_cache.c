/**
 * @file
 * @brief VkDescriptorSetLayout cache: manages the reuse of descriptor set layouts if they have the same bindings
 */

#include "vulcain.h"
#include "vc_private.h"



/**
 * @brief Hashes a descriptor set layout create info
 *
 * @param d
 * @return Hash
 * TODO: Check if this hash is good (this was made with random shit thrown arround)
 */
u64    _vc_priv_vk_descriptor_set_layout_info_hash(VkDescriptorSetLayoutCreateInfo *d, u64 size)
{
    ASSERT( size == sizeof(VkDescriptorSetLayoutCreateInfo) );
    ASSERT(d->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);


    u64 result = d->bindingCount;

    for (int i = 0; i < d->bindingCount; i++)
    {
        VkDescriptorSetLayoutBinding b = d->pBindings[i];
        u64 binding_hash               = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

        result ^= binding_hash;
    }

    return result;
}

void                     vc_priv_descriptor_set_layout_cache_create(vc_descriptor_set_layout_cache   *cache)
{
    hashmap_create(&cache->set_layout_map, VC_PRIV_DESCRIPTOR_SET_LAYOUT_CACHE_BUCKET_COUNT, sizeof(VkDescriptorSetLayoutCreateInfo), sizeof(VkDescriptorSetLayout), (hashmap_hash_func)_vc_priv_vk_descriptor_set_layout_info_hash);
    cache->layouts = darray_create(VkDescriptorSetLayout);
}

VkDescriptorSetLayout    _vc_priv_descriptor_set_layout_add(vc_ctx *ctx, vc_descriptor_set_layout_cache *cache, VkDescriptorSetLayoutCreateInfo info)
{
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    vkCreateDescriptorSetLayout(ctx->vk_device, &info, NULL, &layout);
    if(layout == VK_NULL_HANDLE)
        return layout; // Prevents adding a null handle to hashmap

    hashmap_insert(&cache->set_layout_map, &info, &layout);
    darray_push(cache->layouts, layout);
    return layout;
}

VkDescriptorSetLayout    vc_priv_descriptor_set_layout_cache_grab(vc_ctx *ctx, vc_descriptor_set_layout_cache *cache, VkDescriptorSetLayoutCreateInfo info)
{
    void *data = hashmap_lookup(&cache->set_layout_map, &info);

    if(data)
    {
        return *(VkDescriptorSetLayout *)data;
    }

    return _vc_priv_descriptor_set_layout_add(ctx, cache, info);
}

void    vc_priv_descriptor_set_layout_cache_destroy(vc_ctx *ctx, vc_descriptor_set_layout_cache *cache)
{
    u32 layout_count = darray_length(cache->layouts);
    for(int i = 0; i < layout_count; i++)
    {
        vkDestroyDescriptorSetLayout(ctx->vk_device, cache->layouts[i], NULL);
    }

    darray_destroy(cache->layouts);
    hashmap_destroy(&cache->set_layout_map);
}

