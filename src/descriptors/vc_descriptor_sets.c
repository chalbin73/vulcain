#include "../vulcain.h"
#include "../handles/vc_internal_types.h"
#include "vc_ds_alloc.h"
#include "../base/data_structures/darray.h"

vc_descriptor_set
vc_descriptor_set_allocate(vc_ctx *ctx, vc_descriptor_set_layout layout)
{
    _vc_descriptor_set_layout_intern *sl_i = vc_handles_manager_deref(&ctx->handles_manager, layout);

    VkDescriptorSet set = vc_ds_alloc_allocate(&ctx->ds_allocator, ctx->current_device, sl_i->layout);

    _vc_descriptor_set_intern set_i =
    {
        .set    = set,
        .layout = sl_i->layout,
    };
    vc_descriptor_set hndl = vc_handles_manager_walloc(&ctx->handles_manager, VC_HANDLE_DESCRIPTOR_SET, &set_i);

    return hndl;
}


void
_vc_descriptor_set_writer_init(vc_descriptor_set_writer   *writer)
{
    writer->writes = darray_create(VkWriteDescriptorSet);

    writer->img_infos = darray_create(VkDescriptorImageInfo);
    writer->buf_infos = darray_create(VkDescriptorBufferInfo);
}

void
vc_descriptor_set_writer_write_image(vc_ctx *ctx, vc_descriptor_set_writer *writer, u32 binding, u32 array_elt, vc_image_view view, vc_handle sampler, VkImageLayout layout, VkDescriptorType image_type)
{
    _vc_image_view_intern *img_vw_i = vc_handles_manager_deref(&ctx->handles_manager, view);
    if(writer->writes == NULL)
    {
        _vc_descriptor_set_writer_init(writer);
    }

    VkDescriptorImageInfo info =
    {
        .sampler     = VK_NULL_HANDLE, // TODO:
        .imageView   = img_vw_i->view,
        .imageLayout = layout,
    };
    darray_push(writer->img_infos, info);

    VkWriteDescriptorSet write =
    {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pImageInfo      = &writer->img_infos[darray_length(writer->img_infos) - 1],
        .descriptorType  = image_type,
        .dstBinding      = binding,
        .dstArrayElement = array_elt,
        .descriptorCount = 1,
    };

    darray_push(writer->writes, write);
}

void
vc_descriptor_set_writer_write_buffer(vc_ctx *ctx, vc_descriptor_set_writer *writer, u32 binding, u32 array_elt, vc_buffer buffer, u64 offset, u64 range, VkDescriptorType buffer_type)
{
    _vc_buffer_intern *buf_i = vc_handles_manager_deref(&ctx->handles_manager, buffer);
    if(writer->writes == NULL)
    {
        _vc_descriptor_set_writer_init(writer);
    }

    VkDescriptorBufferInfo info =
    {
        .range  = range,
        .offset = offset,
        .buffer = buf_i->buffer,
    };

    darray_push(writer->buf_infos, info);

    VkWriteDescriptorSet write =
    {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pBufferInfo     = &writer->buf_infos[darray_length(writer->buf_infos) - 1],
        .descriptorType  = buffer_type,
        .dstBinding      = binding,
        .dstArrayElement = array_elt,
        .descriptorCount = 1,
    };

    darray_push(writer->writes, write);
}

void
vc_descriptor_set_writer_write(vc_ctx *ctx, vc_descriptor_set_writer *writer, vc_descriptor_set set)
{
    _vc_descriptor_set_intern *set_i = vc_handles_manager_deref(&ctx->handles_manager, set);
    u32 len                          = darray_length(writer->writes);
    for(u32 i = 0; i < len; i++)
    {
        writer->writes[i].dstSet = set_i->set;
    }
    vkUpdateDescriptorSets(ctx->current_device, darray_length(writer->writes), writer->writes, 0, 0);

    darray_destroy(writer->writes);
    darray_destroy(writer->buf_infos);
    darray_destroy(writer->img_infos);
}

