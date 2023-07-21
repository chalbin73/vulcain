#include "vc_managed_types.h"
#include "vulcain.h"

/* ---------------- Command buffer object relative functions ---------------- */

b8                   _vc_priv_command_buffer_destroy(vc_ctx *ctx, vc_priv_man_command_buffer *buffer)
{
    vkFreeCommandBuffers(ctx->vk_device, buffer->pool, 1, &buffer->command_buffer);
    return TRUE;
}

vc_command_buffer    vc_command_buffer_main_create(vc_ctx *ctx, vc_queue_type queue)
{
    VkCommandBufferAllocateInfo alloc_ci =
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandBufferCount = 1,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool        = ctx->queues.pools[queue],
    };

    vc_priv_man_command_buffer buf;
    buf.queue_type = queue;
    buf.pool       = alloc_ci.commandPool;
    VK_CHECKH(vkAllocateCommandBuffers(ctx->vk_device, &alloc_ci, &buf.command_buffer), "Could not allocate a command buffer in one of the main pools.");
    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_COMMAND_BUFFER, (vc_man_destroy_func)_vc_priv_command_buffer_destroy);
    return vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_COMMAND_BUFFER, &buf);
}

void    vc_command_buffer_reset(vc_ctx *ctx, vc_command_buffer command_buffer)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vkResetCommandBuffer(buf->command_buffer, 0);
}

void    vc_command_buffer_submit(vc_ctx *ctx, vc_command_buffer command_buffer, vc_semaphore wait_on_semaphore, VkPipelineStageFlags *wait_stages)
{
    VkSemaphore semaphore = VK_NULL_HANDLE;

    if (wait_on_semaphore != VC_NULL_HANDLE)
    {
        vc_priv_man_semaphore *sem = vc_handle_mgr_ptr(&ctx->handle_manager, wait_on_semaphore);
        semaphore = sem->semaphore;
    }
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);

    VkSubmitInfo submit_i =
    {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &buf->command_buffer,
        .signalSemaphoreCount = 0,
        .waitSemaphoreCount   = (wait_on_semaphore == VC_NULL_HANDLE) ? 0 : 1,
        .pWaitSemaphores      = &semaphore,
        .pWaitDstStageMask    = wait_stages,
    };



    vkQueueSubmit(ctx->queues.queues[buf->queue_type], 1, &submit_i, VK_NULL_HANDLE);
}

void    vc_command_buffer_begin(vc_ctx *ctx, vc_command_buffer command_buffer)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);

    VkCommandBufferBeginInfo begin_i =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(buf->command_buffer, &begin_i);
}

void    vc_command_buffer_end(vc_ctx *ctx, vc_command_buffer command_buffer)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vkEndCommandBuffer(buf->command_buffer);
}

/* ---------------- COMMANDS ---------------- */
/* ---------------- Pipelines relative commands ---------------- */

void    vc_command_buffer_compute_pipeline(vc_ctx *ctx, vc_command_buffer command_buffer, compute_dispatch_desc *desc)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vc_priv_man_compute_pipe *pipe  = vc_handle_mgr_ptr(&ctx->handle_manager, desc->pipe);

    vkCmdBindPipeline(buf->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipe->pipeline);
    vkCmdDispatch(buf->command_buffer, desc->groups_x, desc->groups_y, desc->groups_z);
}

/* ---------------- Descriptors relative commands ---------------- */

void    vc_command_buffer_bind_descriptor_set(vc_ctx *ctx, vc_command_buffer command_buffer, vc_handle pipeline, vc_descriptor_set desc_set)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vc_priv_man_descriptor_set *set = vc_handle_mgr_ptr(&ctx->handle_manager, desc_set);
    void *pipeline_obj              = vc_handle_mgr_ptr(&ctx->handle_manager, pipeline);
    VkPipelineLayout layout         = VK_NULL_HANDLE;
    VkPipelineBindPoint bind_point  = 0;

    if (*( (vc_pipeline_type *)pipeline_obj ) == VC_PIPELINE_TYPE_COMPUTE)
    {
        vc_priv_man_compute_pipe *comp_pipe = pipeline_obj;
        layout     = comp_pipe->layout;
        bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
    }
    else if (*( (vc_pipeline_type *)pipeline_obj ) == VC_PIPELINE_TYPE_GRAPHICS)
    {
        vc_priv_man_graphics_pipe *grap_pipe = pipeline_obj;
        bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
        layout     = grap_pipe->layout;
    }
    else
    {
        FATAL("Pipeline object error.");
        return;
    }

    vkCmdBindDescriptorSets(
        buf->command_buffer,
        bind_point,
        layout,
        0,
        1,
        &set->set,
        0,
        NULL
        );
}

void    vc_command_render_pass_begin(vc_ctx *ctx, vc_command_buffer command_buffer, render_pass_begin_desc desc)
{
    vc_priv_man_command_buffer *buf      = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vc_priv_man_render_pass *render_pass = vc_handle_mgr_ptr(&ctx->handle_manager, desc.pass);
    vc_priv_man_framebuffer *fb          = vc_handle_mgr_ptr(&ctx->handle_manager, desc.frambuffer);

    VkRenderPassBeginInfo begin_i =
    {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass  = render_pass->render_pass,
        .framebuffer = fb->frambuffer,

        .renderArea      = desc.render_area,
        .clearValueCount = desc.clear_value_count,
        .pClearValues    = desc.clear_values
    };

    vkCmdBeginRenderPass(buf->command_buffer, &begin_i, desc.subpass_contents);
}

void    vc_command_pipeline_bind(vc_ctx *ctx, vc_command_buffer command_buffer, vc_handle pipe)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    void *pipeline_obj              = vc_handle_mgr_ptr(&ctx->handle_manager, pipe);

    if (*( (vc_pipeline_type *)pipeline_obj ) == VC_PIPELINE_TYPE_COMPUTE)
    {
        vc_priv_man_compute_pipe *comp_pipe = pipeline_obj;
        vkCmdBindPipeline(buf->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, comp_pipe->pipeline);
    }
    else if (*( (vc_pipeline_type *)pipeline_obj ) == VC_PIPELINE_TYPE_GRAPHICS)
    {
        vc_priv_man_graphics_pipe *grap_pipe = pipeline_obj;
        vkCmdBindPipeline(buf->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, grap_pipe->pipeline);
        return;
    }
    else
    {
        FATAL("Pipeline object error.");
        return;
    }
}

void    vc_command_render_pass_end(vc_ctx *ctx, vc_command_buffer command_buffer)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vkCmdEndRenderPass(buf->command_buffer);
}

void    vc_command_dyn_set_viewport(vc_ctx *ctx, vc_command_buffer command_buffer, u32 viewport_count, VkViewport *viewports)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vkCmdSetViewport(buf->command_buffer, 0, viewport_count, viewports);
}

void    vc_command_dyn_set_scissors(vc_ctx *ctx, vc_command_buffer command_buffer, u32 scissor_count, VkRect2D *scissors)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vkCmdSetScissor(buf->command_buffer, 0, scissor_count, scissors);
}

void    vc_command_draw(vc_ctx *ctx, vc_command_buffer command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vkCmdDraw(buf->command_buffer, vertex_count, instance_count, first_vertex, first_instance);
}

void    vc_command_draw_indexed(vc_ctx *ctx, vc_command_buffer command_buffer, u32 index_count, u32 instance_count, u32 first_index, int32_t vertex_offset, u32 first_instance)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vkCmdDrawIndexed(buf->command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void    vc_command_bind_vertex_buffer(vc_ctx *ctx, vc_command_buffer command_buffer, vc_buffer buffer, u32 binding, u64 offset)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vc_priv_man_buffer *vert_buf    = vc_handle_mgr_ptr(&ctx->handle_manager, buffer);

    vkCmdBindVertexBuffers(buf->command_buffer, binding, 1, &vert_buf->buffer, &offset);
}

void    vc_command_bind_index_buffer(vc_ctx *ctx, vc_command_buffer command_buffer, vc_buffer buffer, u64 offset, VkIndexType index_type)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vc_priv_man_buffer *index_buf   = vc_handle_mgr_ptr(&ctx->handle_manager, buffer);

    vkCmdBindIndexBuffer(buf->command_buffer, index_buf->buffer, offset, index_type);
}
