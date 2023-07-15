#include "vc_managed_types.h"
#include "vulcain.h"

b8           _vc_priv_buffer_destroy(vc_ctx *ctx, vc_priv_man_buffer *buffer)
{
    vmaDestroyBuffer(ctx->vma_allocator, buffer->buffer, buffer->allocation);
    return TRUE;
}

vc_buffer    vc_buffer_allocate(vc_ctx *ctx, buffer_alloc_desc alloc_desc)
{
    u32 queue_indices[VC_QUEUE_TYPE_COUNT];
    vc_queue_flags_to_queue_indices_list(ctx, alloc_desc.queues, queue_indices);

    VkBufferCreateInfo buf_ci =
    {
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size                  = alloc_desc.size,
        .usage                 = alloc_desc.buffer_usage,
        .sharingMode           = (alloc_desc.share ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE), // TODO: Make this customizable
        .queueFamilyIndexCount = vc_u32_flags_set_bits(alloc_desc.queues),
        .pQueueFamilyIndices   = queue_indices,

    };

    VmaAllocationCreateInfo alloc_ci =
    {
        .flags         = (alloc_desc.require_host_visible) ? (0) : 0,
        .usage         = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags =
            ( (alloc_desc.require_host_visible) ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) : 0 ) |
            ( (alloc_desc.require_device_local) ? (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) : 0 ),
        .priority      = 1.0f,
    };

    vc_priv_man_buffer man_buf;
    VK_CHECKH(vmaCreateBuffer(ctx->vma_allocator, &buf_ci, &alloc_ci, &man_buf.buffer, &man_buf.allocation, NULL), "Could not allocate a buffer (VMA).");
    man_buf.size = alloc_desc.size;

    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_BUFFER, (vc_man_destroy_func)_vc_priv_buffer_destroy);
    return vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_BUFFER, &man_buf);
}