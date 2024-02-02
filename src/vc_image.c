#include "handles/vc_internal_types.h"
#include "vulcain.h"
#include "vc_enum_util.h"
#include <alloca.h>


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

    VK_CHECKH(vmaCreateImage(ctx->main_allocator, &img_ci, &alloc_ci, &img.image, &img.alloc, NULL), "Could not allocate an image");

    vc_image hndl = vc_handles_manager_walloc(&ctx->handles_manager, VC_HANDLE_IMAGE, &img);

    return hndl;
}

