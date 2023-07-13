#include "vc_managed_types.h"
#include "vulcain.h"

u64    vc_priv_hash_vk_descriptor_set_layout(VkDescriptorSetLayoutCreateInfo   *d)
{
    ASSERT(d->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
    u64 hash = 0b1111111100101101101000110111110111010011111100000011010011101011; // Random start

    hash ^= ( (u64)d->flags ) >> 32;
    hash ^= d->bindingCount;

    for (int i = 0; i < d->bindingCount; i++)
    {
        u64 sub_hash = d->pBindings[i].binding * (d->pBindings[i].descriptorType * 2513447010837);
        sub_hash ^= ( (u64)d->pBindings[i].descriptorType ) >> 32;
        sub_hash *= ( (u64)d->pBindings[i].stageFlags * 35343747910837 );
        sub_hash ^= d->pBindings[i].descriptorCount >> 16;
        hash     ^= sub_hash;
    }

    return hash;
}

b8                          _vc_priv_desc_set_layout_destroy(vc_ctx *ctx, vc_priv_man_descriptor_set_layout *set)
{
    hashmap_remove(&ctx->desc_set_layouts_hashmap, &set->hash);
    vkDestroyDescriptorSetLayout(ctx->vk_device, set->set_layout, NULL);
    return TRUE;
}

vc_descriptor_set_layout    vc_priv_desc_set_layout_get(vc_ctx *ctx, VkDescriptorSetLayoutCreateInfo *ci)
{
    u64 ci_hash = vc_priv_hash_vk_descriptor_set_layout(ci);
    void *res   = hashmap_lookup(&ctx->desc_set_layouts_hashmap, &ci_hash);

    if (res == NULL)
    {
        // Desc layout does not already exist, needs to be created.
        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        VK_CHECKH(vkCreateDescriptorSetLayout(ctx->vk_device, ci, NULL, &layout), "Could not create a descriptor set layout");

        vc_priv_man_descriptor_set_layout set_layout =
        {
            .set_layout = layout,
            .hash       = ci_hash,
        };

        vc_descriptor_set_layout hndl = vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_DESCRIPTOR_SET_LAYOUT, &set_layout);
        vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_DESCRIPTOR_SET_LAYOUT, (vc_man_destroy_func)_vc_priv_desc_set_layout_destroy);
        hashmap_insert(&ctx->desc_set_layouts_hashmap, &ci_hash, &hndl);
        return hndl;
    }
    // Desc layout already exists, return.
    return *( (vc_descriptor_set_layout *)res );
}