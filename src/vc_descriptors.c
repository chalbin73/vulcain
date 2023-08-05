#include "vc_managed_types.h"
#include "vc_private.h"
#include "vulcain.h"

b8                          _vc_priv_descriptor_set_layout_destroy(vc_ctx *ctx, vc_priv_man_descriptor_set_layout *layout)
{
    // NOOP Because descriptor set layouts are managed by the cache
    return TRUE;
}

vc_descriptor_set_layout    vc_descriptor_set_layout_create(vc_ctx *ctx, descriptor_set_desc desc_set_desc)
{
    VkDescriptorSetLayoutBinding *bindings = mem_allocate(sizeof(VkDescriptorSetLayoutBinding) * desc_set_desc.binding_count, MEMORY_TAG_RENDERER);

    for (int i = 0; i < desc_set_desc.binding_count; i++)
    {
        bindings[i] = (VkDescriptorSetLayoutBinding)
        {
            .binding         = i,
            .descriptorType  = desc_set_desc.bindings[i].descriptor_type,
            .descriptorCount = desc_set_desc.bindings[i].descriptor_count,
            .stageFlags      = desc_set_desc.bindings[i].stage_flags,
        };
    }
    VkDescriptorSetLayoutCreateInfo dsl_ci =
    {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = desc_set_desc.binding_count,
        .pBindings    = bindings,
    };

    VkDescriptorSetLayout layout                 = vc_priv_descriptor_set_layout_cache_grab(ctx, &ctx->set_layout_cache, dsl_ci);
    vc_priv_man_descriptor_set_layout man_layout =
    {
        .set_layout = layout,
    };

    vc_descriptor_set_layout hndl = vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_DESCRIPTOR_SET_LAYOUT, &man_layout);
    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_DESCRIPTOR_SET_LAYOUT, (vc_man_destroy_func)_vc_priv_descriptor_set_layout_destroy);

    mem_free(bindings);
    return hndl;
}

b8                   _vc_priv_descriptor_set_destroy(vc_ctx *ctx, vc_priv_man_descriptor_set *set)
{
    vkFreeDescriptorSets(ctx->vk_device, set->parent_pool, 1, &set->set);
    return TRUE;
}

vc_descriptor_set    vc_descriptor_set_create(vc_ctx *ctx, descriptor_set_desc desc_set_desc)
{
    // We can create set layout on the fly :
    //   If the user creates it before this call it is gonna be grabed by the cache
    //   If the user wants to create it after this call, it will be already in the cache anyways
    //  => So useless to ask the user for it
    vc_descriptor_set_layout set_layout = vc_descriptor_set_layout_create(ctx, desc_set_desc);

    vc_priv_man_descriptor_set_layout *man_set_layout = vc_handle_mgr_ptr(&ctx->handle_manager, set_layout);
    vc_priv_man_descriptor_set set                    =
    {
        .layout_hash = man_set_layout->hash,
    };
    vc_priv_descriptor_set_allocator_alloc(ctx, &ctx->set_allocator, man_set_layout->set_layout, &set.set, &set.parent_pool);

    // Update descriptors
    VkWriteDescriptorSet *writes = mem_allocate(sizeof(VkWriteDescriptorSet) * desc_set_desc.binding_count, MEMORY_TAG_RENDERER);

    for (int i = 0; i < desc_set_desc.binding_count; i++)
    {
        writes[i] = (VkWriteDescriptorSet)
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = set.set,
            .dstBinding      = i,
            .descriptorCount = desc_set_desc.bindings[i].descriptor_count,
            .descriptorType  = desc_set_desc.bindings[i].descriptor_type,
        };

        if (desc_set_desc.bindings[i].buffer_info)
        {
            descriptor_binding_buffer *buffer_binding = desc_set_desc.bindings[i].buffer_info;
            vc_priv_man_buffer *buf_info              = vc_handle_mgr_ptr(&ctx->handle_manager, buffer_binding->buffer);

            writes[i].pBufferInfo = &(VkDescriptorBufferInfo)
            {
                .buffer = buf_info->buffer,
                .offset = (buffer_binding->whole_buffer) ? 0 : buffer_binding->offset,
                .range  = (buffer_binding->whole_buffer) ? buf_info->size : buffer_binding->range,
            };
        }
        if (desc_set_desc.bindings[i].image_info)
        {
            descriptor_binding_image *image_binding = desc_set_desc.bindings[i].image_info;
            writes[i].pImageInfo = &(VkDescriptorImageInfo)
            {
                .sampler = image_binding->sampler ?
                           ( (vc_priv_man_image_sampler *)vc_handle_mgr_ptr(&ctx->handle_manager, image_binding->sampler) )->sampler : VK_NULL_HANDLE,
                .imageView = image_binding->image_view ?
                             ( (vc_priv_man_image_view *)vc_handle_mgr_ptr(&ctx->handle_manager, image_binding->image_view) )->image_view : VK_NULL_HANDLE,
                .imageLayout = image_binding->layout,
            };
        }
    }
    vkUpdateDescriptorSets(ctx->vk_device, desc_set_desc.binding_count, writes, 0, NULL);
    mem_free(writes);

    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_DESCRIPTOR_SET, (vc_man_destroy_func)_vc_priv_descriptor_set_destroy);
    return vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_DESCRIPTOR_SET, &set);
}

// Rural

