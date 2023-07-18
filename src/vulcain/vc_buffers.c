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
        .flags         = 0,
        .usage         = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = alloc_desc.required_properties,
        .priority      = 1.0f,
    };

    vc_priv_man_buffer man_buf;
    man_buf.memory_properties = alloc_desc.required_properties;
    VK_CHECKH(vmaCreateBuffer(ctx->vma_allocator, &buf_ci, &alloc_ci, &man_buf.buffer, &man_buf.allocation, NULL), "Could not allocate a buffer (VMA).");
    man_buf.size = alloc_desc.size;

    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_BUFFER, (vc_man_destroy_func)_vc_priv_buffer_destroy);
    return vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_BUFFER, &man_buf);
}

void    vc_buffer_map(vc_ctx *ctx, vc_buffer buffer, void **mapped)
{
    vc_priv_man_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, buffer);
    if(buf->memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        vmaMapMemory(ctx->vma_allocator, buf->allocation, mapped);
        return;
    }
    *mapped = NULL;
}

void    vc_buffer_unmap(vc_ctx *ctx, vc_buffer buffer)
{
    vc_priv_man_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, buffer);
    vmaUnmapMemory(ctx->vma_allocator, buf->allocation);
}

void    vc_buffer_coherent_staged_write(vc_ctx *ctx, vc_buffer dest, u64 offset, u64 length, void *data, vc_queue_type copy_queue)
{
    vc_priv_man_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, dest);
    if( (buf->memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && (buf->memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) )
    {
        void *map = NULL;
        vc_buffer_map(ctx, dest, &map);

        void *offseted_map = (void *)( (u64)map + offset );

        mem_memcpy(offseted_map, data, length);

        vc_buffer_unmap(ctx, dest);
        return;
    }
    else
    {
        vc_buffer staging_buffer = vc_buffer_allocate(
            ctx,
            (buffer_alloc_desc)
            {
                .buffer_usage        = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .required_properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                .share               = FALSE,
                .size                = length,
            }
            );

        vc_buffer_coherent_staged_write(ctx, staging_buffer, 0, length, data, copy_queue);

        vc_command_buffer command_buffer = vc_command_buffer_main_create(ctx, copy_queue);
        vc_command_buffer_begin(ctx, command_buffer);

        VkBufferCopy copy =
        {
            .srcOffset = 0,
            .dstOffset = offset,
            .size      = length,
        };
        vc_command_buffer_copy(ctx, command_buffer, staging_buffer, dest, 1, &copy);

        vc_command_buffer_end(ctx, command_buffer);
        vc_command_buffer_submit(ctx, command_buffer, VC_NULL_HANDLE, NULL);
        return;
    }
}

void    vc_command_buffer_copy(vc_ctx *ctx, vc_command_buffer cmd_buf, vc_buffer src, vc_buffer dst, u32 region_count, VkBufferCopy *regions)
{
    vc_priv_man_buffer *src_buf     = vc_handle_mgr_ptr(&ctx->handle_manager, src);
    vc_priv_man_buffer *dst_buf     = vc_handle_mgr_ptr(&ctx->handle_manager, dst);
    vc_priv_man_command_buffer *cmd = vc_handle_mgr_ptr(&ctx->handle_manager, cmd_buf);
    vkCmdCopyBuffer(cmd->command_buffer, src_buf->buffer, dst_buf->buffer, region_count, regions);
}