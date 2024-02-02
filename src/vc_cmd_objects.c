// Everything about command buffers and pools

#include "handles/vc_internal_types.h"
#include "vulcain.h"
#include "vc_enum_util.h"

void
_vc_command_pool_destroy(vc_ctx *ctx, _vc_command_pool_intern *p)
{
    vkDestroyCommandPool(ctx->current_device, p->pool, NULL);
}

vc_command_pool
vc_command_pool_create(vc_ctx *ctx, vc_queue parent_queue, VkCommandPoolCreateFlags flags)
{
    _vc_queue_intern *q           = vc_handles_manager_deref(&ctx->handles_manager, parent_queue);
    VkCommandPoolCreateInfo cp_ci =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = NULL,
        .flags            = flags,
        .queueFamilyIndex = q->queue_family_index,
    };

    _vc_command_pool_intern pool_struct =
    {
        0
    };
    VK_CHECKH(vkCreateCommandPool(ctx->current_device, &cp_ci, NULL, &pool_struct.pool), "Could not create a command pool.");

    pool_struct.family_index = q->queue_family_index;

    vc_command_pool hndl = vc_handles_manager_walloc(&ctx->handles_manager, VC_HANDLE_COMMAND_POOL, &pool_struct);
    vc_handles_manager_set_destroy_function(&ctx->handles_manager, VC_HANDLE_COMMAND_POOL, (vc_handle_destroy_func)_vc_command_pool_destroy);

    return hndl;
}

vc_command_buffer
vc_command_buffer_allocate(vc_ctx *ctx, VkCommandBufferLevel level, vc_command_pool pool)
{
    _vc_command_pool_intern *pool_struct = vc_handles_manager_deref(&ctx->handles_manager, pool);
    VkCommandBufferAllocateInfo cb_ai    =
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level              = level,
        .commandPool        = pool_struct->pool,
        .commandBufferCount = 1,
    };

    _vc_command_buffer_intern buf_intern =
    {
        0
    };

    VK_CHECKH(vkAllocateCommandBuffers(ctx->current_device, &cb_ai, &buf_intern.buffer), "Could not allocate a command buffer.");

    vc_command_buffer hndl = vc_handles_manager_walloc(&ctx->handles_manager, VC_HANDLE_COMMAND_BUFFER, &buf_intern);

    return hndl;
}

void
vc_command_buffer_reset(vc_ctx *ctx, vc_command_buffer buffer, VkCommandBufferResetFlags reset_flags)
{
    _vc_command_buffer_intern *buf = vc_handles_manager_deref(&ctx->handles_manager, buffer);

    vkResetCommandBuffer(buf->buffer, reset_flags);
}

