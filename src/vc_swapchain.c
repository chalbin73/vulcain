#include "vc_enum_util.h"
#include "handles/vc_internal_types.h"
#include <alloca.h>

void _vc_swapchain_rebuild(vc_ctx *ctx, vc_swapchain hndl, _vc_swapchain_intern *s);
void _vc_swapchain_destroy(vc_ctx *ctx, _vc_swapchain_intern *s);

vc_swapchain
vc_swapchain_create(vc_ctx                                   *ctx,
                    vc_windowing_system                       win_sys,
                    VkImageUsageFlags                         image_usage,
                    vc_format_query                           query,
                    vc_swapchain_creation_callback_func       create_clbk,
                    vc_swapchain_destruction_callback_func    destroy_clbk,
                    void                                     *clbk_udata)
{
    vc_debug("Creating a swapchain with windowing system '%s'.", win_sys.windowing_system_name);


    // Handle creation
    vc_swapchain swap       = vc_handles_manager_alloc(&ctx->handles_manager, VC_HANDLE_SWAPCHAIN);
    _vc_swapchain_intern *s = vc_handles_manager_deref(&ctx->handles_manager, swap);

    vc_handles_manager_set_destroy_function(&ctx->handles_manager, VC_HANDLE_SWAPCHAIN, (vc_handle_destroy_func)_vc_swapchain_destroy);

    s->swapchain = VK_NULL_HANDLE; // Signifies that no previous swapchain existed

    s->creation_callback    = create_clbk;
    s->destruction_callback = destroy_clbk;
    s->clbk_udata           = clbk_udata;
    s->windowing_system     = win_sys;
    s->image_usage          = image_usage;

    // Create the surface on which the swapchain will present
    VK_CHECKH(
        win_sys.create_surface(
            ctx->vk_instance,
            win_sys.udata,
            NULL,
            &s->surface
            ),
        "Could not create swapchain, because surface creation failed."
        );

    // Query available formats
    u32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->current_physical_device, s->surface, &format_count, NULL);
    VkSurfaceFormatKHR *surface_formats = alloca(sizeof(VkSurfaceFormatKHR) * format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->current_physical_device, s->surface, &format_count, surface_formats);

    // Make a format set out of supported formats
    VkFormat *formats = alloca(sizeof(VkFormat) * format_count);
    vc_debug("Swapchain supports following formats :");
    for(u32 i = 0; i < format_count; i++)
    {
        vc_debug(
            "\tformats=%-40s -- color_space=%s",
            vc_priv_VkFormat_to_str(surface_formats[i].format),
            vc_priv_VkColorSpaceKHR_to_str(surface_formats[i].colorSpace)
            );
        formats[i] = surface_formats[i].format;
    }

    vc_format_set set =
    {
        .formats      = formats,
        .format_count = format_count,
    };

    // Query format based on user query
    // TODO: Take into account image_usage (but should be inputed by query)
    u32 selected_format = 0;
    b8 format_found     = vc_format_query_index(ctx, query, set, &selected_format);

    if(!format_found)
    {
        vc_error("ERROR: Swapchain creation failed, no supporting format found.");
        return VC_NULL_HANDLE;
    }

    // Save format
    s->surface_format = surface_formats[selected_format];

    vc_info(
        "Format selected for swapchain : format='%s' -- color_space='%s'.",
        vc_priv_VkFormat_to_str(s->surface_format.format),
        vc_priv_VkColorSpaceKHR_to_str(s->surface_format.colorSpace)
        );

    vc_debug("Swapchain init finished. Building the swapchain.");
    _vc_swapchain_rebuild(ctx, swap, s);

    return swap;
}


/**
 * @brief Queries based on the windowing system what the swapchain size should be
 *
 * @param ctx A pointer to the vulcain ctx
 * @param s The swapchain internal structure representation
 * @param img_count[out] where to store the chosen number of images
 * @return The correct extent.
 */
VkExtent2D
_vc_swapchain_get_extent(vc_ctx *ctx, _vc_swapchain_intern *s, u32 *img_count)
{
    VkSurfaceCapabilitiesKHR caps =
    {
        0
    };
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->current_physical_device, s->surface, &caps);
    *img_count = caps.minImageCount;

    if(caps.currentExtent.width != U32_MAX)
    {
        return caps.currentExtent;
    }


    u32 ws_width  = 0;
    u32 ws_height = 0;
    s->windowing_system.get_fb_size(s->windowing_system.udata, &ws_width, &ws_height);

    VkExtent2D ext =
    {
        .width  = ws_width,
        .height = ws_height,
    };

    // Clamp
    ext.width  = CLAMP(ext.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    ext.height = CLAMP(ext.height, caps.minImageExtent.height, caps.maxImageExtent.height);

    return ext;
}

void _vc_swapchain_build(vc_ctx *ctx, _vc_swapchain_intern *s);

void
_vc_swapchain_rebuild(vc_ctx *ctx, vc_swapchain hndl, _vc_swapchain_intern *s)
{
    vc_debug("Swapchain rebuild/build requested.");
    // Check if old swapchain already exists.
    if(s->swapchain != VK_NULL_HANDLE)
    {
        vc_debug("Destroying previous swapchain.");
        //  Free handles
        for(u32 i = 0; i < s->image_count; i++)
        {
            vc_handles_manager_destroy_handle(&ctx->handles_manager, s->swapchain_images[i]);
        }
        //  Call destroy callback
        s->destruction_callback(ctx, s->clbk_udata);
        //  Destroy swapchain and objs
        vkDestroySwapchainKHR(ctx->current_device, s->swapchain, NULL);
    }

    vc_debug("Beginning swapchain build. Querying extent and image count :");
    // Create new swapchain
    vc_swapchain_created_info c_info =
    {
        0
    };

    c_info.swapchain_extent       = _vc_swapchain_get_extent(ctx, s, &c_info.swapchain_image_count);
    c_info.swapchain              = hndl;
    c_info.swapchain_image_format = s->surface_format.format;
    vc_debug("\textent = %dx%d", c_info.swapchain_extent.width, c_info.swapchain_extent.height);
    vc_debug("\tmin_image_count = %d", c_info.swapchain_image_count);

    s->image_count  = c_info.swapchain_image_count;
    s->image_extent = c_info.swapchain_extent;

    //  Create actual swapchain object
    _vc_swapchain_build(ctx, s);

    c_info.swapchain_image_count = s->image_count;
    c_info.images                = s->swapchain_images;
    vc_debug("Creation of swapchain object successful. image_count=%d", s->image_count);
    //  Create underlying objects
    //  Call create callback
    s->creation_callback(
        ctx,
        s->clbk_udata,
        c_info
        );
}

void
_vc_swapchain_build(vc_ctx *ctx, _vc_swapchain_intern *s)
{
    VkSwapchainCreateInfoKHR swapchain_ci =
    {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = s->surface,
        .minImageCount         = s->image_count,
        .imageFormat           = s->surface_format.format,
        .imageColorSpace       = s->surface_format.colorSpace,
        .imageExtent           = s->image_extent,
        .imageArrayLayers      = 1,
        .imageUsage            = s->image_usage,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .preTransform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = VK_PRESENT_MODE_FIFO_KHR,
        .clipped               = FALSE,
        .oldSwapchain          = VK_NULL_HANDLE,
    };

    VK_CHECK(vkCreateSwapchainKHR(ctx->current_device, &swapchain_ci, NULL, &s->swapchain), "Swapchain object creation failed.");

    // Get image handles
    vkGetSwapchainImagesKHR(ctx->current_device, s->swapchain, &s->image_count, NULL);
    s->swapchain_images = mem_allocate(sizeof(vc_image) * s->image_count, MEMORY_TAG_RENDERER);
    VkImage *images = alloca(sizeof(VkImage) * s->image_count);
    vkGetSwapchainImagesKHR(ctx->current_device, s->swapchain, &s->image_count, images);

    for(u32 i = 0; i < s->image_count; i++)
    {
        _vc_image_intern img_intern =
        {
            .externally_managed = TRUE,
            .image              = images[i],
            .alloc              = VK_NULL_HANDLE,
        };

        s->swapchain_images[i] = vc_handles_manager_walloc(&ctx->handles_manager, VC_HANDLE_IMAGE, &img_intern);
    }
}

void
_vc_swapchain_destroy(vc_ctx *ctx, _vc_swapchain_intern *s)
{
    if(s->swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(ctx->current_device, s->swapchain, NULL);
    }

    vkDestroySurfaceKHR(ctx->vk_instance, s->surface, NULL);
}

vc_swpchn_img_id
vc_swapchain_acquire_image(vc_ctx *ctx, vc_swapchain swapchain, vc_semaphore signal_semaphore)
{
    _vc_swapchain_intern *swp = vc_handles_manager_deref(&ctx->handles_manager, swapchain);
    _vc_semaphore_intern *sem = vc_handles_manager_deref(&ctx->handles_manager, signal_semaphore);
    u32 img_id                = 0;
    VK_CHECK(
        vkAcquireNextImageKHR(ctx->current_device, swp->swapchain, UINT64_MAX, sem->semaphore, VK_NULL_HANDLE, &img_id),
        "Could not acquire next image in a swapchain"
        );
    return img_id;
}

void
vc_swapchain_present_image(vc_ctx *ctx, vc_swapchain swapchain, vc_queue presentation_queue, vc_semaphore wait_semaphore, vc_swpchn_img_id image_id)
{
    _vc_swapchain_intern *swp = vc_handles_manager_deref(&ctx->handles_manager, swapchain);
    _vc_semaphore_intern *sem = vc_handles_manager_deref(&ctx->handles_manager, wait_semaphore);
    _vc_queue_intern *que     = vc_handles_manager_deref(&ctx->handles_manager, presentation_queue);

    VkResult present_result;
    VkPresentInfoKHR info =
    {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &sem->semaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &swp->swapchain,
        .pImageIndices      = &image_id,
        .pResults           = &present_result,
    };

    vkQueuePresentKHR(que->queue, &info);
}

vc_image
vc_swapchain_get_image(vc_ctx *ctx, vc_swapchain swapchain, vc_swpchn_img_id index)
{
    _vc_swapchain_intern *swp = vc_handles_manager_deref(&ctx->handles_manager, swapchain);
    return swp->swapchain_images[index];
}

