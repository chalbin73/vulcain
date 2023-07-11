#include "../base/base.h"
#include "vc_managed_types.h"
#include "vulcain.h"
#include <vulkan/vulkan_core.h>

b8 _vc_priv_image_destroy(vc_ctx *ctx, vc_priv_man_image *image)
{
    if (!image->external)
    {
        vmaDestroyImage(ctx->vma_allocator, image->image, image->allocation);
    }
    return TRUE;
}

void vc_command_image_pipe_barrier(vc_ctx *ctx, vc_command_buffer command_buffer, vc_image image,
                                   VkImageLayout src_layout, VkImageLayout dst_layout,
                                   VkPipelineStageFlags from, VkPipelineStageFlags to,
                                   VkAccessFlags src_access, VkAccessFlags dst_access,
                                   vc_queue_type src_queue, vc_queue_type dst_queue)

{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vc_priv_man_image          *img = vc_handle_mgr_ptr(&ctx->handle_manager, image);

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

// Performs a simple, unoptimized layout transition, used when creating/transfering, and should not be used in performance application
// TODO: Make a image transtion routine, that takes an array of images
void vc_image_transition_layout(vc_ctx *ctx, vc_image image, VkImageLayout src_layout, VkImageLayout dst_layout, vc_queue_type queue)
{
    vc_command_buffer buf = vc_command_buffer_main_create(ctx, queue);
    vc_command_buffer_begin(ctx, buf);

    vc_command_image_pipe_barrier(ctx, buf, image, src_layout, dst_layout, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
                                  VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT, VC_QUEUE_IGNORED, VC_QUEUE_IGNORED);
    vc_command_buffer_end(ctx, buf);
    vc_command_buffer_submit(ctx, buf, VC_NULL_HANDLE, NULL);

    vc_queue_wait_idle(ctx, queue);

    vc_handle_destroy(ctx, buf);
}

vc_image vc_image_allocate(vc_ctx *ctx, image_create_desc desc)
{
    VkImageCreateInfo image_ci =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType =
                (desc.image_dimension == 1 ? VK_IMAGE_TYPE_1D : 0) |
                (desc.image_dimension == 2 ? VK_IMAGE_TYPE_2D : 0) |
                (desc.image_dimension == 3 ? VK_IMAGE_TYPE_3D : 0),
            .format = desc.image_format,
            .extent = (VkExtent3D){.width = desc.width, .height = desc.height, .depth = desc.depth},
            .mipLevels = desc.mip_levels == 0 ? 1 : desc.mip_levels,
            .arrayLayers = desc.layers == 0 ? 1 : desc.layers,
            .samples = desc.sample_count,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = desc.image_usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo alloc_ci =
        {
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .priority = 1.0f,
        };

    if (desc.mip_levels == VC_IMAGE_CREATE_AUTO_MIP)
    {
        u32 max_comp = MAX(MAX(desc.width, desc.height), desc.depth);
        u32 log2 = 0;

        // Compute log2
        while (max_comp >>= 1)
        {
            log2++;
        }

        // The number of mip levels is the base level, plus the number of time we can divide the image size by 2
        log2++;
        image_ci.mipLevels = log2;
    }

    u32 queue_indices[VC_QUEUE_TYPE_COUNT];

    if (desc.share)
    {
        u32 queue_share_count = 0;
        if (desc.queues & VC_QUEUE_MAIN_BIT)
        {
            queue_indices[queue_share_count] = ctx->queues.indices[VC_QUEUE_MAIN];
            queue_share_count++;
        }

        if (desc.queues & VC_QUEUE_COMPUTE_BIT)
        {
            queue_indices[queue_share_count] = ctx->queues.indices[VC_QUEUE_COMPUTE];
            queue_share_count++;
        }

        if (desc.queues & VC_QUEUE_TRANSFER_BIT)
        {
            queue_indices[queue_share_count] = ctx->queues.indices[VC_QUEUE_TRANSFER];
            queue_share_count++;
        }

        image_ci.sharingMode = (queue_share_count == 0 || queue_share_count == 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
        image_ci.queueFamilyIndexCount = queue_share_count;
        image_ci.pQueueFamilyIndices = queue_indices;
    }

    vc_priv_man_image img;
    VK_CHECKH(vmaCreateImage(ctx->vma_allocator, &image_ci, &alloc_ci, &img.image, &img.allocation, NULL), "Could not allocate an image on device.");
    img.external = FALSE;

    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_IMAGE, (vc_man_destroy_func)_vc_priv_image_destroy);
    vc_image img_hndl = vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_IMAGE, &img);

    if (desc.layout != VK_IMAGE_LAYOUT_UNDEFINED)
    {
        vc_image_transition_layout(ctx, img_hndl, VK_IMAGE_LAYOUT_UNDEFINED, desc.layout, VC_QUEUE_MAIN); // TODO: Select valid queue
    }

    return img_hndl;
}