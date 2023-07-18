#include "vulcain.h"
#include "vc_managed_types.h"
#include "vc_private.h"

//TODO: Find a cleaner way to do this
b8      _vc_priv_image_destroy(vc_ctx *ctx, vc_priv_man_image *image);

void    vc_swapchain_setup(vc_ctx *ctx, swapchain_desc desc)
{
    _vc_priv_setup_default_swapchain(ctx, desc);
    ctx->swapchain.desc = desc;
}

b8      _vc_priv_setup_default_swapchain(vc_ctx *ctx, swapchain_desc desc)
{
    if (!ctx->use_windowing)
    {
        FATAL("Cannot setup a swapchain without a windowing system.");
        return FALSE;
    }
    INFO("Selecting swapchain configuration");

    _vc_priv_select_swapchain_configuration(ctx, desc);
    VkExtent2D swp_extent =
    {
        0
    };
    _vc_priv_get_optimal_swapchain_size(ctx, desc, &swp_extent);
    INFO("Create swapchain with extent of %ux%u", swp_extent.width, swp_extent.height);
    _vc_priv_create_swapchain(ctx, desc, swp_extent);

    INFO("Swapchain created, calling user callback.");
    desc.recreation_callback(
        ctx,
        desc.callback_user_data,
        (swapchain_created_info){ .width = swp_extent.width, .height = swp_extent.height }
        );

    return TRUE;
}

b8    _vc_priv_create_swapchain(vc_ctx *ctx, swapchain_desc desc, VkExtent2D extent)
{
    VkSwapchainCreateInfoKHR sc_ci =
    {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = ctx->vk_window_surface,
        .minImageCount    = ctx->swapchain_conf.image_count,
        .imageFormat      = ctx->swapchain_conf.swapchain_format.format,
        .imageColorSpace  = ctx->swapchain_conf.swapchain_format.colorSpace,
        .imageExtent      = (VkExtent2D){ .width = extent.width,              .height= extent.height },
        .imageArrayLayers = 1,
        .imageUsage       = desc.swapchain_images_usage,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform     = ctx->swapchain_conf.capabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .clipped          = VK_FALSE,
    };

    ctx->swapchain_conf.swapchain_extent = extent;

    VK_CHECKR(vkCreateSwapchainKHR(ctx->vk_device, &sc_ci, NULL, &ctx->swapchain.vk_swapchain), "Could not create swapchain.");
    TRACE("Created swapchain successfully.");

    vkGetSwapchainImagesKHR(ctx->vk_device, ctx->swapchain.vk_swapchain, &ctx->swapchain.swapchain_image_count, NULL);
    ctx->swapchain.swapchain_image_views = mem_allocate(sizeof(VkImageView) * ctx->swapchain.swapchain_image_count, MEMORY_TAG_RENDERER);
    ctx->swapchain.swapchain_images      = mem_allocate(sizeof(VkImage) * ctx->swapchain.swapchain_image_count, MEMORY_TAG_RENDERER);
    ctx->swapchain.swapchain_image_hndls = mem_allocate(sizeof(vc_image) * ctx->swapchain.swapchain_image_count, MEMORY_TAG_RENDERER);
    vkGetSwapchainImagesKHR(ctx->vk_device, ctx->swapchain.vk_swapchain, &ctx->swapchain.swapchain_image_count, ctx->swapchain.swapchain_images);
    TRACE("Retrieved swapchain images.");

    vc_command_buffer cmd_buf = vc_command_buffer_main_create(ctx, VC_QUEUE_MAIN);
    vc_command_buffer_begin(ctx, cmd_buf);

    for (int i = 0; i < ctx->swapchain.swapchain_image_count; i++)
    {
        vc_priv_man_image img =
        {
            .external   = TRUE,
            .image      = ctx->swapchain.swapchain_images[i],
            .allocation = VK_NULL_HANDLE,
            .image_desc =
            {
                .image_dimension = 2,
                .image_format    = ctx->swapchain_conf.swapchain_format.format,
                .width           = ctx->swapchain_conf.swapchain_extent.width,
                .height          = ctx->swapchain_conf.swapchain_extent.height,
                .depth           = 1,
                .mip_levels      = 1,
                .layer_count     = 1,
                .sample_count    = VK_SAMPLE_COUNT_1_BIT,
                .image_usage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .share           = FALSE,
                .layout          = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            },
            .full_image_view     = VK_NULL_HANDLE,
        };
        ctx->swapchain.swapchain_image_hndls[i] = vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_IMAGE, &img);

        vc_command_image_pipe_barrier(
            ctx,
            cmd_buf,
            ctx->swapchain.swapchain_image_hndls[i],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_ACCESS_MEMORY_WRITE_BIT,
            VK_ACCESS_MEMORY_WRITE_BIT,
            VC_QUEUE_MAIN,
            VC_QUEUE_MAIN
            );
    }
    vc_command_buffer_end(ctx, cmd_buf);
    vc_command_buffer_submit(ctx, cmd_buf, VC_NULL_HANDLE, 0);
    vc_queue_wait_idle(ctx, VC_QUEUE_MAIN);
    vc_handle_destroy(ctx, cmd_buf);

    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_IMAGE, (vc_man_destroy_func)_vc_priv_image_destroy); //TODO: Find a cleaner way to do this

    vc_swapchain_create_full_image_views(ctx); // NOTE: This might not be a good idea ...
    TRACE("Created swapchain image views.");
    return TRUE;
}

b8    _vc_priv_delete_swapchain(vc_ctx   *ctx)
{
    vkDestroySwapchainKHR(ctx->vk_device, ctx->swapchain.vk_swapchain, NULL);

    mem_free(ctx->swapchain.swapchain_image_views);
    mem_free(ctx->swapchain.swapchain_images);
    mem_free(ctx->swapchain.swapchain_image_hndls);

    return TRUE;
}

b8    _vc_priv_select_swapchain_configuration(vc_ctx *ctx, swapchain_desc desc)
{
    // Select swapchain format
    u32 format_count = 0;
    VkSurfaceFormatKHR *formats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &format_count, NULL);
    formats = mem_allocate(format_count * sizeof(VkSurfaceFormatKHR), MEMORY_TAG_RENDERER);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &format_count, formats);

    /* ---------------- Swapchain format ---------------- */
    ctx->swapchain_conf.swapchain_format = formats[0]; // Fallback
    b8 found_format = FALSE;
    TRACE("Supported surface formats :");
    for (int i = 0; i < format_count; i++)
    {
        TRACE( "\t%s - %s,", vc_priv_VkFormat_to_str(formats[i].format), vc_priv_VkColorSpaceKHR_to_str(formats[i].colorSpace) );
        if ( (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB || formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
        {
            ctx->swapchain_conf.swapchain_format = formats[i];
            found_format                         = TRUE;
            break;
        }
    }
    if(!found_format)
    {
        ERROR(
            "Not found requested format, using fallback format : format=%s, color_space=%s",
            vc_priv_VkFormat_to_str(ctx->swapchain_conf.swapchain_format.format),
            vc_priv_VkColorSpaceKHR_to_str(ctx->swapchain_conf.swapchain_format.colorSpace)
            );
    }
    else
    {
        TRACE(
            "Found a format for the swapchain : format=%s, color_space=%s",
            vc_priv_VkFormat_to_str(ctx->swapchain_conf.swapchain_format.format),
            vc_priv_VkColorSpaceKHR_to_str(ctx->swapchain_conf.swapchain_format.colorSpace)
            );
    }
    mem_free(formats);

    /* ---------------- Present mode ---------------- */
    u32 present_mode_count = 0;
    VkPresentModeKHR *present_modes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &present_mode_count, NULL);
    present_modes = mem_allocate(sizeof(VkPresentModeKHR) * present_mode_count, MEMORY_TAG_RENDERER);
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &present_mode_count, present_modes);

    ctx->swapchain_conf.present_mode = VK_PRESENT_MODE_FIFO_KHR; // FIFO Is always available
    for (int i = 0; i < present_mode_count; i++)
    {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            ctx->swapchain_conf.present_mode = present_modes[i];
            INFO("Got mailbox present mode.");
        }
    }
    mem_free(present_modes);

    VkSurfaceCapabilitiesKHR capa;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &capa);

    /* ---------------- Image count ---------------- */
    ctx->swapchain_conf.image_count = capa.minImageCount + 1;

    if (capa.maxImageCount != 0) // If not infinite available images, cap
    {
        ctx->swapchain_conf.image_count = CLAMP(ctx->swapchain_conf.image_count, capa.minImageCount, capa.maxImageCount);
    }
    ctx->swapchain_conf.capabilities = capa;

    TRACE("Using %d swapchain images.", ctx->swapchain_conf.image_count);

    /* ---------------- Depth buffer format ---------------- */
    u32 candidates_count         = 3;
    VkFormat depth_candidates[3] =
    {
        VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT
    };
    b8 found = FALSE;
    for (int i = 0; i < candidates_count; i++)
    {
        VkFormatProperties props =
        {
            0
        };
        vkGetPhysicalDeviceFormatProperties(ctx->vk_selected_physical_device, depth_candidates[i], &props);

        if ( (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT )
        {
            ctx->swapchain_conf.depth_format = depth_candidates[i];
            found                            = TRUE;
        }
    }
    if (!found)
    {
        WARN("No possible depth format found.");
    }
    TRACE(
        "Depth format: %s",
        vc_priv_VkFormat_to_str(ctx->swapchain_conf.depth_format)
        );

    return TRUE;
}

b8    _vc_priv_get_optimal_swapchain_size(vc_ctx *ctx, swapchain_desc desc, VkExtent2D *extent)
{
    // Select extent
    u32 width  = 0;
    u32 height = 0;
    ctx->windowing_system.get_framebuffer_size_fun(ctx->windowing_system.windowing_ctx, &width, &height);

    if (ctx->swapchain_conf.capabilities.maxImageExtent.width == U32_MAX)
    {
        extent->width  = CLAMP(width, ctx->swapchain_conf.capabilities.minImageExtent.width, ctx->swapchain_conf.capabilities.maxImageExtent.width);
        extent->height = CLAMP(height, ctx->swapchain_conf.capabilities.minImageExtent.height, ctx->swapchain_conf.capabilities.maxImageExtent.height);
    }
    else
    {
        *extent = ctx->swapchain_conf.capabilities.currentExtent;
    }
    return TRUE;
}

void        vc_swapchain_create_full_image_views(vc_ctx   *ctx)
{
    u32 count = vc_swapchain_image_count(ctx);
    for(int i = 0; i < count; i++)
    {
        vc_image_create_full_image_view(ctx, vc_swapchain_get_image_hndls(ctx)[i]);
    }

}

void        vc_swapchain_acquire_image(vc_ctx *ctx, u32 *image_id, vc_semaphore acquired_semaphore)
{
    vc_priv_man_semaphore *sem = vc_handle_mgr_ptr(&ctx->handle_manager, acquired_semaphore);
    vkAcquireNextImageKHR(ctx->vk_device, ctx->swapchain.vk_swapchain, U64_MAX, sem->semaphore, VK_NULL_HANDLE, image_id);
}

void        vc_swapchain_present_image(vc_ctx *ctx, u32 image_id)
{
    vkQueuePresentKHR(
        ctx->queues.queues[VC_QUEUE_MAIN],
        &(VkPresentInfoKHR){
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pImageIndices      = &image_id,
            .pSwapchains        = &ctx->swapchain.vk_swapchain,
            .swapchainCount     = 1,
            .waitSemaphoreCount = 0,

        }
        );
}

vc_image   *vc_swapchain_get_image_hndls(vc_ctx   *ctx)
{
    return ctx->swapchain.swapchain_image_hndls;
}

u32         vc_swapchain_image_count(vc_ctx   *ctx)
{
    return ctx->swapchain.swapchain_image_count;
}