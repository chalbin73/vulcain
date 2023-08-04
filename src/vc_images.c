#include "base.h"
#include "vc_private.h"
#include "vc_managed_types.h"
#include "vulcain.h"
#include <vulkan/vulkan_core.h>

b8      _vc_priv_image_destroy(vc_ctx *ctx, vc_priv_man_image *image)
{
    if(!image->external)
    {
        vmaDestroyImage(ctx->vma_allocator, image->image, image->allocation);
    }
    return TRUE;
}

void    vc_command_image_pipe_barrier(vc_ctx                    *ctx,
                                      vc_command_buffer          command_buffer,
                                      vc_image                   image,

                                      VkImageLayout              src_layout,
                                      VkImageLayout              dst_layout,

                                      VkPipelineStageFlags       from,
                                      VkPipelineStageFlags       to,

                                      VkAccessFlags              src_access,
                                      VkAccessFlags              dst_access,

                                      vc_queue_type              src_queue,
                                      vc_queue_type              dst_queue,

                                      VkImageSubresourceRange    subresource_range
                                      )

{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vc_priv_man_image *img          = vc_handle_mgr_ptr(&ctx->handle_manager, image);

    VkImageMemoryBarrier bar =
    {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image               = img->image,
        .subresourceRange    = subresource_range,
        .oldLayout           = src_layout,
        .newLayout           = dst_layout,
        .srcAccessMask       = src_access,
        .dstAccessMask       = dst_access,
        .srcQueueFamilyIndex = (src_queue == VC_QUEUE_IGNORED) ? VK_QUEUE_FAMILY_IGNORED : ctx->queues.indices[src_queue],
        .dstQueueFamilyIndex = (dst_queue == VC_QUEUE_IGNORED) ? VK_QUEUE_FAMILY_IGNORED : ctx->queues.indices[dst_queue],
    };

    vkCmdPipelineBarrier(buf->command_buffer, from, to, 0, 0, NULL, 0, NULL, 1, &bar);
    img->image_desc.layout = bar.newLayout;
}

void    vc_command_simple_image_copy(vc_ctx              *ctx,
                                     vc_command_buffer    command_buffer,
                                     vc_image             src,
                                     vc_image             dst)
{
    vc_priv_man_image *src_img          = vc_handle_mgr_ptr(&ctx->handle_manager, src);
    vc_priv_man_image *dst_img          = vc_handle_mgr_ptr(&ctx->handle_manager, dst);
    vc_priv_man_command_buffer *cmd_buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);

    VkImageCopy copy =
    {
        .srcSubresource     =
        {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT, // TODO: Make this configurable
            .baseArrayLayer = 0,
            .layerCount     = dst_img->image_desc.layer_count,
            .mipLevel       = 0, // TODO: Make this configurable (maybe mutliple levels at once)
        },
        .srcOffset      = { 0 },
        .dstSubresource =
        {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT, // TODO: Make this configurable
            .baseArrayLayer = 0,
            .layerCount     = dst_img->image_desc.layer_count,
            .mipLevel       = 0, // TODO: Make this configurable (maybe mutliple levels at once)
        },
        .dstOffset = { 0 },
        .extent    = { src_img->image_desc.width,    src_img->image_desc.height,src_img->image_desc.depth },
    };
    vkCmdCopyImage(cmd_buf->command_buffer, src_img->image, src_img->image_desc.layout, dst_img->image, dst_img->image_desc.layout, 1, &copy);
}

// Performs a simple, unoptimized layout transition, used when creating/transfering, and should not be used in performance application
// TODO: Make a image transtion routine, that takes an array of images
void    vc_image_transition_layout(vc_ctx *ctx, vc_image image, VkImageLayout src_layout, VkImageLayout dst_layout, vc_queue_type queue, VkImageAspectFlags aspect)
{
    vc_command_buffer buf  = vc_command_buffer_main_create(ctx, queue, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    vc_priv_man_image *img = vc_handle_mgr_ptr(&ctx->handle_manager, image);
    vc_command_buffer_begin(ctx, buf);

    vc_command_image_pipe_barrier(
        ctx,
        buf,
        image,
        src_layout,
        dst_layout,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
        VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
        VC_QUEUE_IGNORED,
        VC_QUEUE_IGNORED,
        (VkImageSubresourceRange)
        {
            .aspectMask     = aspect,
            .layerCount     = img->image_desc.layer_count,
            .levelCount     = img->image_desc.mip_levels,
            .baseMipLevel   = 0,
            .baseArrayLayer = 0,
        }
        );
    vc_command_buffer_end(ctx, buf);
    vc_command_buffer_submit(ctx, buf, VC_NULL_HANDLE, NULL);

    vc_queue_wait_idle(ctx, queue);

    vc_handle_destroy(ctx, buf);
}

vc_image    vc_image_allocate(vc_ctx *ctx, image_create_desc desc)
{
    VkImageCreateInfo image_ci =
    {
        .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType =
            (desc.image_dimension == 1 ? VK_IMAGE_TYPE_1D : 0) |
            (desc.image_dimension == 2 ? VK_IMAGE_TYPE_2D : 0) |
            (desc.image_dimension == 3 ? VK_IMAGE_TYPE_3D : 0),
        .format = desc.image_format,
        .extent =
            (VkExtent3D)
        {
            .width  = desc.width,
            .height = desc.height,
            .depth  = desc.depth
        },
        .mipLevels     = desc.mip_levels == 0 ? 1 : desc.mip_levels,
        .arrayLayers   = desc.layer_count == 0 ? 1 : desc.layer_count,
        .samples       = desc.sample_count == 0 ? VK_SAMPLE_COUNT_1_BIT : desc.sample_count,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = desc.image_usage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo alloc_ci =
    {
        .usage    = VMA_MEMORY_USAGE_GPU_ONLY,
        .priority = 1.0f,
    };

    if (desc.mip_levels == VC_IMAGE_CREATE_AUTO_MIP)
    {
        u32 max_comp = MAX(MAX(desc.width, desc.height), desc.depth);
        u32 log2     = 0;

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

        image_ci.sharingMode           = (queue_share_count == 0 || queue_share_count == 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
        image_ci.queueFamilyIndexCount = queue_share_count;
        image_ci.pQueueFamilyIndices   = queue_indices;
    }

    vc_priv_man_image img;
    VK_CHECKH(vmaCreateImage(ctx->vma_allocator, &image_ci, &alloc_ci, &img.image, &img.allocation, NULL), "Could not allocate an image on device.");

    img.external   = FALSE;
    img.image_desc = desc;

    img.image_desc.mip_levels  = image_ci.mipLevels; // Because of VC_IMAGE_CREATE_AUTO_MIP
    img.image_desc.layer_count = image_ci.arrayLayers; // Because of null value previously handled

    // Create/setup handle
    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_IMAGE, (vc_man_destroy_func)_vc_priv_image_destroy);
    vc_image img_hndl = vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_IMAGE, &img);

    return img_hndl;
}

b8               _vc_priv_image_view_destroy(vc_ctx *ctx, vc_priv_man_image_view *img)
{
    vkDestroyImageView(ctx->vk_device, img->image_view, NULL);
    return TRUE;
}

vc_image_view    vc_image_view_create(vc_ctx *ctx, vc_image image, image_view_desc desc)
{
    vc_priv_man_image *img        = vc_handle_mgr_ptr(&ctx->handle_manager, image);
    VkImageViewCreateInfo view_ci =
    {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = img->image,
        .viewType         = desc.view_type,
        .format           = img->image_desc.image_format,
        .subresourceRange = desc.subresource_range,
        .components       = desc.component_mapping,
    };


    vc_priv_man_image_view view_man =
    {
        0
    };
    VK_CHECKH(vkCreateImageView(ctx->vk_device, &view_ci, NULL, &view_man.image_view), "Could not create a image view.");
    view_man.parent_image = image;

    vc_image_view hndl = vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_IMAGE_VIEW, &view_man);
    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_IMAGE_VIEW, (vc_man_destroy_func)_vc_priv_image_view_destroy);
    return hndl;
}


b8                  _vc_priv_image_sampler_destroy(vc_ctx *ctx, vc_priv_man_image_sampler *img)
{
    vkDestroySampler(ctx->vk_device, img->sampler, NULL);
    return TRUE;
}

vc_image_sampler    vc_image_sampler_create(vc_ctx *ctx, sampler_desc desc)
{
    VkSamplerCreateInfo sampler_ci =
    {
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter               = desc.mag_filter,
        .minFilter               = desc.min_filter,
        .mipmapMode              = desc.mipmap_mode,
        .addressModeU            = desc.address_mode_u,
        .addressModeV            = desc.address_mode_v,
        .addressModeW            = desc.address_mode_w,
        .mipLodBias              = desc.mip_lod_bias,
        .anisotropyEnable        = desc.anisotropy_enable,
        .maxAnisotropy           = desc.max_anisotropy,
        .compareEnable           = desc.compare_enable,
        .compareOp               = desc.compare_op,
        .minLod                  = desc.min_lod,
        .maxLod                  = desc.max_lod,
        .borderColor             = desc.border_color,
        .unnormalizedCoordinates = desc.unnormalized_coordinates,
    };

    vc_priv_man_image_sampler man_sampler;
    VK_CHECKH(vkCreateSampler(ctx->vk_device, &sampler_ci, NULL, &man_sampler.sampler), "Could not create a sampler.");

    vc_image_sampler hndl = vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_IMAGE_SAMPLER, &man_sampler);
    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_IMAGE_SAMPLER, (vc_man_destroy_func)_vc_priv_image_sampler_destroy);
    return hndl;
}

void    vc_image_fill_from_buffer(vc_ctx *ctx, vc_image img, vc_buffer src, VkImageLayout transitioned_layout, VkImageAspectFlags aspect_dst, vc_queue_type queue)
{
    vc_priv_man_image *dst_img = vc_handle_mgr_ptr(&ctx->handle_manager, img);
    //vc_priv_man_buffer *src_buf = vc_handle_mgr_ptr(&ctx->handle_manager, src);

    vc_command_buffer buf = vc_command_buffer_main_create(ctx, queue, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    vc_command_buffer_begin(ctx, buf);

    vc_command_image_pipe_barrier(
        ctx,
        buf,
        img,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
        VK_ACCESS_MEMORY_WRITE_BIT,
        VC_QUEUE_IGNORED,
        VC_QUEUE_IGNORED,
        (VkImageSubresourceRange)
        {
            .aspectMask     = aspect_dst,
            .layerCount     = dst_img->image_desc.layer_count,
            .levelCount     = dst_img->image_desc.mip_levels,
            .baseMipLevel   = 0,
            .baseArrayLayer = 0,
        }
        );

    VkBufferImageCopy region =
    {
        .imageExtent       = (VkExtent3D){ dst_img->image_desc.width, dst_img->image_desc.height, dst_img->image_desc.depth },
        .imageOffset       = (VkOffset3D){ 0 },
        .bufferOffset      = 0,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = (VkImageSubresourceLayers)
        {
            .mipLevel       = 0,
            .aspectMask     = aspect_dst,
            .layerCount     = 1,
            .baseArrayLayer = 0,
        }
    };

    vc_command_copy_buffer_to_image(ctx, buf, src, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    vc_command_image_pipe_barrier(
        ctx,
        buf,
        img,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        transitioned_layout,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_ACCESS_MEMORY_WRITE_BIT,
        VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
        VC_QUEUE_IGNORED,
        VC_QUEUE_IGNORED,
        (VkImageSubresourceRange)
        {
            .aspectMask     = aspect_dst,
            .layerCount     = dst_img->image_desc.layer_count,
            .levelCount     = dst_img->image_desc.mip_levels,
            .baseMipLevel   = 0,
            .baseArrayLayer = 0,
        }
        );
    vc_command_buffer_end(ctx, buf);
    vc_command_buffer_submit(ctx, buf, VC_NULL_HANDLE, NULL);

    vc_queue_wait_idle(ctx, queue);
}
