#include "vulcain.h"
#include "vc_private.h"
#include "base.h"

// Such a routine is needed as everytime the swapchain is recreated it probably changed size, this procedure gets the new size
void    _vc_priv_swapchain_get_optimal_extent(vc_ctx *ctx, VkExtent2D *swp_extent)
{
    u32 width  = 0;
    u32 height = 0;

    // Ask windowing system
    ctx->windowing_system.get_framebuffer_size_fun(ctx->windowing_system.windowing_ctx, &width, &height);

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &ctx->swapchain_conf.capabilities);

    if(ctx->swapchain_conf.capabilities.currentExtent.width == U32_MAX)
    {
        swp_extent->width  = CLAMP( width, ctx->swapchain_conf.capabilities.minImageExtent.width, ctx->swapchain_conf.capabilities.maxImageExtent.width);
        swp_extent->height = CLAMP( height, ctx->swapchain_conf.capabilities.minImageExtent.height, ctx->swapchain_conf.capabilities.maxImageExtent.height);
    }
    else
    {
        *swp_extent = ctx->swapchain_conf.capabilities.currentExtent;
    }
}

// Actual swapchain creation
void    _vc_priv_swapchain_create(vc_ctx *ctx, vc_swapchain_desc desc, VkExtent2D extent)
{
    VkSwapchainCreateInfoKHR sc_ci =
    {
        .sType           = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface         = ctx->vk_window_surface,
        .minImageCount   = ctx->swapchain_conf.image_count,
        .imageFormat     = ctx->swapchain_conf.swapchain_format.format,
        .imageColorSpace = ctx->swapchain_conf.swapchain_format.colorSpace,
        .imageExtent     = (VkExtent2D){
            .width        = extent.width,                                     .height= extent.height
        },
        .imageArrayLayers = 1,
        .imageUsage       = desc.swapchain_images_usage,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform     = ctx->swapchain_conf.capabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .clipped          = VK_FALSE,
        .oldSwapchain     = VK_NULL_HANDLE,
    };

    ctx->swapchain_conf.swapchain_extent = extent;

    VK_CHECK(vkCreateSwapchainKHR(ctx->vk_device, &sc_ci, NULL, &ctx->swapchain.vk_swapchain), "Could not create swapchain.");

    ctx->swapchain.desc = desc;

    vkGetSwapchainImagesKHR(ctx->vk_device, ctx->swapchain.vk_swapchain, &ctx->swapchain.swapchain_image_count, NULL);

    VkImage *swapchain_images = mem_allocate(sizeof(VkImage) * ctx->swapchain.swapchain_image_count, MEMORY_TAG_RENDERER);
    ctx->swapchain.swapchain_image_hndls      = mem_allocate(sizeof(vc_image) * ctx->swapchain.swapchain_image_count, MEMORY_TAG_RENDERER);
    ctx->swapchain.swapchain_image_view_hndls = mem_allocate(sizeof(vc_image_view) * ctx->swapchain.swapchain_image_count, MEMORY_TAG_RENDERER);

    vkGetSwapchainImagesKHR(ctx->vk_device, ctx->swapchain.vk_swapchain, &ctx->swapchain.swapchain_image_count, swapchain_images);

    // Register swapchain's image system in handle system

    vc_command_buffer cmd_buf = vc_command_buffer_main_create(ctx, VC_QUEUE_MAIN, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    vc_command_buffer_begin(ctx, cmd_buf);

    vc_image_view_desc views_desc =
    {
        .subresource_range.baseArrayLayer = 0,
        .subresource_range.baseMipLevel   = 0,
        .subresource_range.levelCount     = 1,
        .subresource_range.layerCount     = 1,
        .subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,

        .component_mapping                = VC_COMPONENT_MAPPING_ID,

        .view_type                        = VK_IMAGE_VIEW_TYPE_2D,
    };

    for (int i = 0; i < ctx->swapchain.swapchain_image_count; i++)
    {

        vc_priv_man_image img =
        {
            .external   = TRUE,
            .image      = swapchain_images[i],
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
        };
        ctx->swapchain.swapchain_image_hndls[i] = vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_IMAGE, &img);

        ctx->swapchain.swapchain_image_view_hndls[i] = vc_image_view_create(ctx, ctx->swapchain.swapchain_image_hndls[i], views_desc);

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
            VC_QUEUE_MAIN,
            (VkImageSubresourceRange)
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseArrayLayer = 0,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .layerCount     = 1,
            }
            );
    }
    vc_command_buffer_end(ctx, cmd_buf);
    vc_command_buffer_submit(ctx, cmd_buf, VC_NULL_HANDLE, 0);
    vc_queue_wait_idle(ctx, VC_QUEUE_MAIN);
    vc_handle_destroy(ctx, cmd_buf);

    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_IMAGE, (vc_man_destroy_func)_vc_priv_image_destroy);
    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_IMAGE_VIEW, (vc_man_destroy_func)_vc_priv_image_view_destroy);

    mem_free(swapchain_images);
}

void    vc_swapchain_commit(vc_ctx *ctx, vc_swapchain_desc desc)
{
    VkExtent2D new_extent;
    _vc_priv_swapchain_get_optimal_extent(ctx, &new_extent);

    _vc_priv_swapchain_create(ctx, desc, new_extent);

    // Call user callbacks

    // Objects created in user callback are dependent on swapchain, they must be marked as such
    vc_handle_mgr_set_current_marker(&ctx->handle_manager, VC_HANDLE_MARKER_SWAPCHAIN_DEPENDENT_BIT);
    if(desc.recreation_callback)
    {
        desc.recreation_callback(
            ctx,
            desc.callback_user_data,
            (vc_swapchain_created_info) { .width = new_extent.width, .height = new_extent.height, .image_count = ctx->swapchain.swapchain_image_count }
            );
    }
    vc_handle_mgr_set_current_marker(&ctx->handle_manager, VC_HANDLE_MARKER_DEFAULT);
    ctx->swapchain.setup = TRUE;

}

void    vc_swapchain_cleanup(vc_ctx   *ctx)
{
    if(!ctx->swapchain.setup)
    {
        return;
    }

    ctx->swapchain.setup = FALSE;

    // Destroy all swapchain dependent objects
    vc_handle_mgr_destroy_marked(&ctx->handle_manager, VC_HANDLE_MARKER_SWAPCHAIN_DEPENDENT_BIT, ctx);

    if(ctx->swapchain.desc.destruction_callback)
    {
        ctx->swapchain.desc.destruction_callback(ctx, ctx->swapchain.desc.callback_user_data);
    }

    vkDestroySwapchainKHR(ctx->vk_device, ctx->swapchain.vk_swapchain, NULL);

    for(int i = 0; i < ctx->swapchain.swapchain_image_count; i++)
    {
        vc_handle_destroy(ctx, ctx->swapchain.swapchain_image_hndls[i]);
        vc_handle_destroy(ctx, ctx->swapchain.swapchain_image_view_hndls[i]);
    }
    mem_free(ctx->swapchain.swapchain_image_hndls);
    mem_free(ctx->swapchain.swapchain_image_view_hndls);

    ctx->swapchain.vk_swapchain          = VK_NULL_HANDLE;
    ctx->swapchain.swapchain_image_hndls = NULL;
    ctx->swapchain.swapchain_image_count = 0;
}

b8    vc_swapchain_acquire_image(vc_ctx *ctx, vc_swp_img_id *image_id, vc_semaphore acquired_semaphore)
{
    vc_priv_man_semaphore *sem = acquired_semaphore ? vc_handle_mgr_ptr(&ctx->handle_manager, acquired_semaphore) : NULL;
    VkResult result            = vkAcquireNextImageKHR(ctx->vk_device, ctx->swapchain.vk_swapchain, U64_MAX, acquired_semaphore ? sem->semaphore : VK_NULL_HANDLE, VK_NULL_HANDLE, image_id);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        // Start recreation after presentation is done
        vkDeviceWaitIdle(ctx->vk_device); // Since presentation is done on main queue

        DEBUG("ACQU: Error");
        vc_swapchain_force_recreation(ctx);
        return FALSE;
    }
    return TRUE;
}

// TODO: Support multiple semaphores
b8    vc_swapchain_present_image(vc_ctx *ctx, u32 image_id, vc_semaphore wait_sem)
{
    VkResult result = VK_SUCCESS;
    vkQueuePresentKHR(
        ctx->queues.queues[VC_QUEUE_MAIN],
        &(VkPresentInfoKHR){
            .sType         = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pImageIndices = &image_id,

            .swapchainCount = 1,
            .pSwapchains    = &ctx->swapchain.vk_swapchain,
            .pResults       = &result,

            .waitSemaphoreCount = wait_sem ? 1 : 0,
            .pWaitSemaphores    = wait_sem ? &( (vc_priv_man_semaphore *)vc_handle_mgr_ptr(&ctx->handle_manager, wait_sem) )->semaphore : NULL,

        }
        );

    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        // Start recreation after presentation is done
        vkDeviceWaitIdle(ctx->vk_device); // Since presentation is done on main queue
        DEBUG("PRESENT: Error");
        vc_swapchain_force_recreation(ctx);
        return FALSE;
    }

    return TRUE;
}

vc_image        *vc_swapchain_get_image_hndls(vc_ctx   *ctx)
{
    return ctx->swapchain.swapchain_image_hndls;
}

vc_image_view   *vc_swapchain_get_image_view_hndls(vc_ctx   *ctx)
{
    return ctx->swapchain.swapchain_image_view_hndls;
}

u32              vc_swapchain_image_count(vc_ctx   *ctx)
{
    return ctx->swapchain.swapchain_image_count;
}

void             vc_swapchain_force_recreation(vc_ctx   *ctx)
{
    vc_swapchain_cleanup(ctx);
    vc_swapchain_commit(ctx, ctx->swapchain.desc);
    DEBUG("Swapchain recreation done.");
}

