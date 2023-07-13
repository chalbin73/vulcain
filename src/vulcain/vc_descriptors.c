#include "vc_managed_types.h"
#include "vulcain.h"

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

    vc_descriptor_set_layout set_layout = vc_priv_desc_set_layout_get(ctx, &dsl_ci);
    mem_free(bindings);
    return set_layout;
}

b8                   _vc_priv_descriptor_set_destroy(vc_ctx *ctx, vc_priv_man_descriptor_set *set)
{
    vkFreeDescriptorSets(ctx->vk_device, ctx->vk_main_descriptor_pool, 1, &set->set);
    return TRUE;
}

vc_descriptor_set    vc_descriptor_set_create(vc_ctx *ctx, descriptor_set_desc desc_set_desc, vc_descriptor_set_layout set_layout)
{
    vc_priv_man_descriptor_set_layout *man_set_layout = vc_handle_mgr_ptr(&ctx->handle_manager, set_layout);

    VkDescriptorSetAllocateInfo alloc_i =
    {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = ctx->vk_main_descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &man_set_layout->set_layout,
    };

    vc_priv_man_descriptor_set set =
    {
        .layout_hash = man_set_layout->hash,
    };

    VK_CHECKH(vkAllocateDescriptorSets(ctx->vk_device, &alloc_i, &set.set), "Could not allocate a descriptor set.");
    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_DESCRIPTOR_SET, (vc_man_destroy_func)_vc_priv_descriptor_set_destroy);

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
            vc_priv_man_image *man_img              = vc_handle_mgr_ptr(&ctx->handle_manager, image_binding->image);
            writes[i].pImageInfo = &(VkDescriptorImageInfo)
            {
                .sampler     = VK_NULL_HANDLE,
                .imageView   = man_img->full_image_view,
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