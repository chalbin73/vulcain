#include "handles/vc_internal_types.h"
#include "vulcain.h"
#include "vc_enum_util.h"
#include <alloca.h>

void
_vc_image_destroy(vc_ctx *ctx, _vc_image_intern *i)
{
    if(!i->externally_managed)
    {
        vmaDestroyImage(ctx->main_allocator, i->image, i->alloc);
    }
}

void
_vc_image_view_destroy(vc_ctx *ctx, _vc_image_view_intern *i)
{
    vkDestroyImageView(ctx->current_device, i->view, NULL);
}

vc_image
vc_image_allocate(vc_ctx *ctx, vc_image_create_info create_info)
{
    VkImageType types[4] =
    {
        [0] = VK_IMAGE_TYPE_1D,
        [1] = VK_IMAGE_TYPE_1D,
        [2] = VK_IMAGE_TYPE_2D,
        [3] = VK_IMAGE_TYPE_3D,
    };

    u32 *conc_queues = NULL;
    if(create_info.queue_count != 0 && !create_info.sharing_exclusive)
    {
        conc_queues = alloca(sizeof(u32) * create_info.queue_count);

        for(u32 i = 0; i < create_info.queue_count; i++)
        {
            _vc_queue_intern *q = vc_handles_manager_deref(&ctx->handles_manager, create_info.queues[i]);
            conc_queues[i] = q->queue_family_index;
        }
    }

    VkImageCreateInfo img_ci =
    {
        .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = types[create_info.image_dimension],
        .format    = create_info.image_format,
        .extent    = (VkExtent3D)
        {
            .width  = create_info.width,
            .height = create_info.height,
            .depth  = create_info.depth,
        },
        .mipLevels   = create_info.mip_level_count,
        .arrayLayers = create_info.array_layer_count,
        .samples     = create_info.sample_count,
        .tiling      = create_info.tiling,
        .usage       = create_info.usage,
        .sharingMode = create_info.sharing_exclusive ?
                       VK_SHARING_MODE_EXCLUSIVE :
                       VK_SHARING_MODE_CONCURRENT,
        .initialLayout         = create_info.initial_layout,
        .queueFamilyIndexCount = create_info.queue_count,
        .pQueueFamilyIndices   = conc_queues,
    };

    VmaAllocationCreateInfo alloc_ci =
    {
        .usage         = create_info.memory.usage,
        .requiredFlags = create_info.memory.mem_props,
        .flags         = create_info.memory.flags,
    };

    _vc_image_intern img =
    {
        0
    };

    img.externally_managed = FALSE;
    img.image_format       = create_info.image_format;

    VK_CHECKH(vmaCreateImage(ctx->main_allocator, &img_ci, &alloc_ci, &img.image, &img.alloc, NULL), "Could not allocate an image");

    vc_image hndl = vc_handles_manager_walloc(&ctx->handles_manager, VC_HANDLE_IMAGE, &img);
    vc_handles_manager_set_destroy_function(&ctx->handles_manager, VC_HANDLE_IMAGE, (vc_handle_destroy_func)_vc_image_destroy);

    return hndl;
}

vc_image_view
vc_image_view_create(vc_ctx *ctx, vc_image image, VkImageViewType type, VkComponentMapping component_map, VkImageSubresourceRange range)
{
    _vc_image_intern *image_i = vc_handles_manager_deref(&ctx->handles_manager, image);

    VkImageViewCreateInfo info =
    {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags            = 0,
        .image            = image_i->image,
        .viewType         = type,
        .format           = image_i->image_format,
        .components       = component_map,
        .subresourceRange = range,
    };

    _vc_image_view_intern view_i =
    {
        0
    };

    VK_CHECKH(vkCreateImageView(ctx->current_device, &info, NULL, &view_i.view), "Could not create an image view.");

    vc_image_view hndl = vc_handles_manager_walloc(&ctx->handles_manager, VC_HANDLE_IMAGE_VIEW, &view_i);
    vc_handles_manager_set_destroy_function(&ctx->handles_manager, VC_HANDLE_IMAGE_VIEW, (vc_handle_destroy_func)_vc_image_view_destroy);

    return hndl;
}

