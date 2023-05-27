#include "vc_managed_types.h"
#include "vulcain.h"
#include "../base/base.h"
#include <vulkan/vulkan_core.h>

b8 _vc_priv_image_destroy(vc_ctx *ctx, vc_priv_man_image *image)
{
    if(!image->external)
    {
        vkDestroyImage(ctx->vk_device, image->image, NULL);
    }
    return TRUE;
}

void vc_cmd_image_pipe_barrier(vc_ctx *ctx, vc_command_buffer command_buffer, vc_image image,
        VkImageLayout src_layout, VkImageLayout dst_layout,
        VkPipelineStageFlags from, VkPipelineStageFlags to,
        VkAccessFlags src_access, VkAccessFlags dst_access,
        vc_queue_type src_queue, vc_queue_type dst_queue)
        
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vc_priv_man_image *img = vc_handle_mgr_ptr(&ctx->handle_manager, image);

    VkImageMemoryBarrier bar = 
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = img->image,
        .subresourceRange = 
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseArrayLayer = 0,
            .baseMipLevel = 0,
            .layerCount = 1,
            .levelCount = 1,
        },
        .oldLayout = src_layout,
        .newLayout = dst_layout,
        .srcAccessMask = src_access,
        .dstAccessMask = dst_access,
        .srcQueueFamilyIndex = (src_queue == VC_QUEUE_IGNORED) ? VK_QUEUE_FAMILY_IGNORED : ctx->queues.indices[src_queue],
        .dstQueueFamilyIndex = (dst_queue == VC_QUEUE_IGNORED) ? VK_QUEUE_FAMILY_IGNORED : ctx->queues.indices[dst_queue],
    };

    vkCmdPipelineBarrier(buf->command_buffer, from, to, 0, 0, NULL, 0, NULL, 1, &bar);
}
