#include "vulcain.h"
#include "handles/vc_internal_types.h"
#include <alloca.h>

vc_cmd_record
vc_command_buffer_begin(vc_ctx *ctx, vc_command_buffer cmd_buffer, VkCommandBufferUsageFlags usage)
{
    _vc_command_buffer_intern *buf = vc_handles_manager_deref(&ctx->handles_manager, cmd_buffer);
    buf->record_ctx = ctx;

    VkCommandBufferBeginInfo begin_i =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags            = usage,
        .pNext            = NULL,
        .pInheritanceInfo = &(VkCommandBufferInheritanceInfo)
        {
            0
        },
    };
    vkBeginCommandBuffer(buf->buffer, &begin_i);

    return (uint64_t)buf;
}

void
vc_command_buffer_end(vc_cmd_record    record)
{
    _vc_command_buffer_intern *buf = (_vc_command_buffer_intern *)record;

    vkEndCommandBuffer(buf->buffer);
}

void
vc_command_buffer_submit(vc_ctx *ctx, vc_command_buffer buffer, vc_queue queue_submit,
                         u32 wait_sem_count, vc_semaphore *wait_sems, VkPipelineStageFlags *wait_stages,
                         u32 signal_sem_count, vc_semaphore *signal_sems)
{
    _vc_command_buffer_intern *buf = vc_handles_manager_deref(&ctx->handles_manager, buffer);
    _vc_queue_intern *q            = vc_handles_manager_deref(&ctx->handles_manager, queue_submit);

    VkSubmitInfo submit_i =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    };

    // Semaphores
    if(wait_sem_count > 0 && wait_sems)
    {
        VkSemaphore *wait_semaphores = alloca(sizeof(VkSemaphore) * wait_sem_count);
        for(u32 i = 0; i < wait_sem_count; i++)
        {
            _vc_semaphore_intern *sem = vc_handles_manager_deref(&ctx->handles_manager, wait_sems[i]);
            wait_semaphores[i] = sem->semaphore;
        }
        submit_i.pWaitSemaphores   = wait_semaphores;
        submit_i.pWaitDstStageMask = wait_stages;
    }

    if(signal_sem_count > 0 && signal_sems)
    {
        VkSemaphore *signal_semaphores = alloca(sizeof(VkSemaphore) * signal_sem_count);
        for(u32 i = 0; i < signal_sem_count; i++)
        {
            _vc_semaphore_intern *sem = vc_handles_manager_deref(&ctx->handles_manager, signal_sems[i]);
            signal_semaphores[i] = sem->semaphore;
        }
        submit_i.pSignalSemaphores = signal_semaphores;
    }

    submit_i.waitSemaphoreCount   = wait_sem_count;
    submit_i.signalSemaphoreCount = signal_sem_count;
    submit_i.commandBufferCount   = 1;
    submit_i.pCommandBuffers      = &buf->buffer;



    vkQueueSubmit(q->queue, 1, &submit_i, VK_NULL_HANDLE); // TODO: Fence support
}

// ## MEMORY COMMANDS

void
vc_cmd_image_barrier(vc_cmd_record record, vc_image image,
                     VkPipelineStageFlags src_stages, VkPipelineStageFlags dst_stages,
                     VkAccessFlags src_access, VkAccessFlags dst_access,
                     VkImageLayout old_layout, VkImageLayout new_layout,
                     VkImageSubresourceRange subres_range,
                     vc_queue _src_queue, vc_queue _dst_queue
                     )
{
    _vc_command_buffer_intern *buf = (_vc_command_buffer_intern *)record;
    _vc_image_intern *img          = vc_handles_manager_deref(&buf->record_ctx->handles_manager, image);

    _vc_queue_intern *src_queue = NULL;
    _vc_queue_intern *dst_queue = NULL;

    if(_src_queue != VC_NULL_HANDLE && _dst_queue != VC_NULL_HANDLE)
    {
        src_queue = vc_handles_manager_deref(&buf->record_ctx->handles_manager, _src_queue);
        dst_queue = vc_handles_manager_deref(&buf->record_ctx->handles_manager, _dst_queue);
    }

    VkImageMemoryBarrier bar =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = img->image,

        .srcAccessMask = src_access,
        .dstAccessMask = dst_access,

        .oldLayout = old_layout,
        .newLayout = new_layout,

        .subresourceRange    = subres_range,

        .srcQueueFamilyIndex = (src_queue == NULL) ? 0 : src_queue->queue_family_index,
        .dstQueueFamilyIndex = (dst_queue == NULL) ? 0 : dst_queue->queue_family_index,
    };

    vkCmdPipelineBarrier(buf->buffer, src_stages, dst_stages, 0, 0, NULL, 0, NULL, 1, &bar);
}

void
vc_cmd_image_clear(vc_cmd_record record, vc_image image,
                   VkImageLayout layout,
                   VkClearColorValue clear_color,
                   VkImageSubresourceRange subres_range)
{
    _vc_command_buffer_intern *buf = (_vc_command_buffer_intern *)record;
    _vc_image_intern *img          = vc_handles_manager_deref(&buf->record_ctx->handles_manager, image);

    vkCmdClearColorImage(buf->buffer, img->image, layout, &clear_color, 1, &subres_range);
}

// Pipeline utils
VkPipeline
_vc_cmd_generic_pipeline_deref(vc_cmd_record record, vc_handle pipeline, VkPipelineBindPoint *bind_point, vc_pipeline_type *type, VkPipelineLayout *layout)
{
    _vc_command_buffer_intern *buf = (_vc_command_buffer_intern *)record;
    vc_pipeline_type *pipe         = vc_handles_manager_deref(&buf->record_ctx->handles_manager, pipeline);

    VkPipelineBindPoint out_bind_point;
    VkPipelineLayout out_layout;
    vc_pipeline_type out_type;
    VkPipeline out_pipeline;

    if(*pipe == VC_PIPELINE_COMPUTE)
    {
        _vc_compute_pipeline_intern *pipe_i = (_vc_compute_pipeline_intern *)pipe;
        out_bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
        out_layout     = pipe_i->layout;
        out_type       = pipe_i->type;
        out_pipeline   = pipe_i->pipeline;
    }
    else if(*pipe == VC_PIPELINE_GRAPHICS)
    {
        _vc_gfx_pipeline_intern *pipe_i = (_vc_gfx_pipeline_intern *)pipe;
        out_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
        out_layout     = pipe_i->layout;
        out_type       = pipe_i->type;
        out_pipeline   = pipe_i->pipeline;
    }
    else
    {
        return VK_NULL_HANDLE;
    }

    if(bind_point)
    {
        *bind_point = out_bind_point;
    }

    if(type)
    {
        *type = out_type;
    }

    if(layout)
    {
        *layout = out_layout;
    }

    return out_pipeline;
}

void
vc_cmd_dispatch_compute(vc_cmd_record record, vc_compute_pipeline pipeline, u32 groups_x, u32 groups_y, u32 groups_z)
{
    _vc_command_buffer_intern *buf    = (_vc_command_buffer_intern *)record;
    _vc_compute_pipeline_intern *pipe = vc_handles_manager_deref(&buf->record_ctx->handles_manager, pipeline);

    vkCmdBindPipeline(buf->buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipe->pipeline);
    vkCmdDispatch(buf->buffer, groups_x, groups_y, groups_z);
}

void
vc_cmd_bind_descriptor_set(vc_cmd_record record, vc_handle pipeline, vc_descriptor_set set, u32 set_dest)
{
    _vc_command_buffer_intern *buf   = (_vc_command_buffer_intern *)record;
    _vc_descriptor_set_intern *set_i = vc_handles_manager_deref(&buf->record_ctx->handles_manager, set);

    VkPipelineLayout layout        = VK_NULL_HANDLE;
    VkPipelineBindPoint bind_point = 0;
    _vc_cmd_generic_pipeline_deref(record, pipeline, &bind_point, NULL, &layout);

    vkCmdBindDescriptorSets(buf->buffer, bind_point, layout, set_dest, 1, &set_i->set, 0, NULL);
}

void
vc_cmd_push_constants(vc_cmd_record record, vc_handle pipeline, VkShaderStageFlags stage, u32 offset, u32 size, void *data)
{
    _vc_command_buffer_intern *buf = (_vc_command_buffer_intern *)record;

    VkPipelineLayout layout = VK_NULL_HANDLE;
    _vc_cmd_generic_pipeline_deref(record, pipeline, NULL, NULL, &layout);

    vkCmdPushConstants(buf->buffer, layout, stage, offset, size, data);
}

// ## DYNAMIC RENDERING ##

VkRenderingAttachmentInfoKHR
_vc_attach_info_to_vk_info(vc_ctx *ctx, vc_rendering_attachment_info info)
{
    _vc_image_view_intern *view = info.image_view == VC_NULL_HANDLE ?
                                  NULL :
                                  vc_handles_manager_deref(&ctx->handles_manager, info.image_view);
    _vc_image_view_intern *resolve_view = info.resolve_image_view == VC_NULL_HANDLE ?
                                          NULL :
                                          vc_handles_manager_deref(&ctx->handles_manager, info.resolve_image_view);

    VkRenderingAttachmentInfoKHR out_info =
    {
        .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView          = view == NULL ? VK_NULL_HANDLE : view->view,
        .imageLayout        = info.image_layout,
        .resolveMode        = info.resolve_mode,
        .resolveImageView   = resolve_view == NULL ? VK_NULL_HANDLE : resolve_view->view,
        .resolveImageLayout = info.resolve_image_layout,
        .loadOp             = info.load_op,
        .storeOp            = info.store_op,
        .clearValue         = info.clear_value,

    };

    return out_info;
}

void
vc_cmd_begin_rendering(vc_cmd_record record, vc_rendering_info info)
{
    _vc_command_buffer_intern *buf = (_vc_command_buffer_intern *)record;

    VkRenderingInfo rend_info =
    {
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext                = NULL,
        .flags                = info.flags,
        .renderArea           = info.render_area,
        .layerCount           = info.layer_count,
        .viewMask             = info.view_mask,
        .colorAttachmentCount = info.color_attachments_count,
        .pColorAttachments    = NULL,
        .pDepthAttachment     = NULL,
        .pStencilAttachment   = NULL,
    };

    VkRenderingAttachmentInfo *color_att_infos = alloca(sizeof(VkRenderingAttachmentInfo) * info.color_attachments_count);

    for(u32 i = 0; i < info.color_attachments_count; i++)
    {
        color_att_infos[i]          = _vc_attach_info_to_vk_info(buf->record_ctx, info.color_attachments[i]);
        rend_info.pColorAttachments = color_att_infos;
    }

    VkRenderingAttachmentInfo stencil_attachment;
    VkRenderingAttachmentInfo depth_attachment;

    if(info.depth_attachment)
    {
        depth_attachment           = _vc_attach_info_to_vk_info(buf->record_ctx, *info.depth_attachment);
        rend_info.pDepthAttachment = &depth_attachment;
    }

    if(info.stencil_attachment)
    {
        stencil_attachment           = _vc_attach_info_to_vk_info(buf->record_ctx, *info.stencil_attachment);
        rend_info.pStencilAttachment = &stencil_attachment;
    }

    vkCmdBeginRendering(buf->buffer, &rend_info);
}

void
vc_cmd_end_rendering(vc_cmd_record    record)
{
    _vc_command_buffer_intern *buf = (_vc_command_buffer_intern *)record;
    vkCmdEndRendering(buf->buffer);
}

void
vc_cmd_draw(vc_cmd_record record, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance)
{
    _vc_command_buffer_intern *buf = (_vc_command_buffer_intern *)record;
    vkCmdDraw(buf->buffer, vertex_count, instance_count, first_vertex, first_instance);
}

void
vc_cmd_bind_pipeline(vc_cmd_record record, vc_gfx_pipeline pipeline)
{
    _vc_command_buffer_intern *buf = (_vc_command_buffer_intern *)record;

    VkPipelineBindPoint bind_point = 0;
    VkPipeline vk_pipeline         = _vc_cmd_generic_pipeline_deref(record, pipeline, &bind_point, NULL, NULL);

    vkCmdBindPipeline(buf->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline);
}

