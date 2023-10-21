#include "vulcain.h"
#include "vc_managed_types.h"
#include "base.h"
#include "vc_private.h"

// This file implements configuration selection routines for the swapchain object

// Configuration selection functions

// FORMAT SELECTION
b8    _vc_priv_swapchain_select_format(vc_ctx *ctx, swapchain_configuration_query query)
{
    VkSurfaceFormatKHR *formats;
    u32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &format_count, NULL);
    if(format_count == 0)
    {
        ERROR("Swapchain does not support any formats.");
        return FALSE;
    }

    formats = mem_allocate(format_count * sizeof(VkSurfaceFormatKHR), MEMORY_TAG_RENDERER);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &format_count, formats);

    INFO("Formats supported by swapchain :");
    for(int i = 0; i < format_count; i++)
    {
        INFO( "\t format=%s - color_space=%s", vc_priv_VkFormat_to_str(formats[i].format), vc_priv_VkColorSpaceKHR_to_str(formats->colorSpace) );
    }

    // Select formats based on query
    format_set format_candidates =
    {
        0
    };
    format_candidates.format_count = format_count;
    format_candidates.formats      = alloca(sizeof(VkFormat) * format_count);

    for(int i = 0; i < format_count; i++)
    {
        format_candidates.formats[i] = formats[i].format;
    }

    u32 result_index;
    b8 found_format = vc_format_query_index(ctx, query.query, format_candidates, &result_index);

    if(!found_format)
    {
        ctx->swapchain_conf.swapchain_format = formats[0];
        ERROR(
            "Not found requested format, using fallback format : format=%s, color_space=%s",
            vc_priv_VkFormat_to_str(ctx->swapchain_conf.swapchain_format.format),
            vc_priv_VkColorSpaceKHR_to_str(ctx->swapchain_conf.swapchain_format.colorSpace)
            );
        return TRUE; //TODO: Make it possible so that user can choose to fail when format isn't found

    }

    ctx->swapchain_conf.swapchain_format = formats[result_index];
    TRACE(
        "Found a format for the swapchain : format=%s, color_space=%s",
        vc_priv_VkFormat_to_str(ctx->swapchain_conf.swapchain_format.format),
        vc_priv_VkColorSpaceKHR_to_str(ctx->swapchain_conf.swapchain_format.colorSpace)
        );

    return TRUE;
}

// PRESENT MODE SELECTION
b8    _vc_priv_swapchain_select_present_mode(vc_ctx *ctx, swapchain_configuration_query query)
{
    u32 present_mode_count          = 0;
    VkPresentModeKHR *present_modes = NULL;

    // Query available present modes
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &present_mode_count, NULL);
    present_modes = alloca(sizeof(VkPresentModeKHR) * present_mode_count);
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
    return TRUE;
}

b8    _vc_priv_swapchain_select_image_count(vc_ctx *ctx, swapchain_configuration_query query)
{
    VkSurfaceCapabilitiesKHR capa;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &capa);

    ctx->swapchain_conf.image_count = capa.minImageCount + 1;

    if (capa.maxImageCount != 0) // If not infinite available images, cap
    {
        ctx->swapchain_conf.image_count = CLAMP(ctx->swapchain_conf.image_count, capa.minImageCount, capa.maxImageCount);
    }
    ctx->swapchain_conf.capabilities = capa;

    TRACE("Using %d swapchain images.", ctx->swapchain_conf.image_count);
    return TRUE;
}

b8    _vc_priv_swapchain_select_depth_format(vc_ctx *ctx, swapchain_configuration_query query)
{
    u32 candidates_count         = 3;
    VkFormat depth_candidates[3] =
    {
        VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT // TODO: Make this configurable
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
        return FALSE; // Do this only if depth is required
    }
    TRACE(
        "Depth format: %s",
        vc_priv_VkFormat_to_str(ctx->swapchain_conf.depth_format)
        );

    return TRUE;
}

b8    vc_swapchain_setup(vc_ctx *ctx, swapchain_configuration_query query)
{
    if(!ctx->use_windowing)
    {
        FATAL("Cannot setup a swapchain without a windowing system.");
        return FALSE;
    }

    INFO("Selecting swapchain configuration.");

    b8 configuration_success = TRUE;

    configuration_success &= _vc_priv_swapchain_select_format(ctx, query);
    configuration_success &= _vc_priv_swapchain_select_present_mode(ctx, query);
    configuration_success &= _vc_priv_swapchain_select_image_count(ctx, query);
    configuration_success &= _vc_priv_swapchain_select_depth_format(ctx, query);

    if(!configuration_success)
    {
        ERROR("Swapchain configuration failed.");
        return FALSE;
    }
    TRACE("Swapchain configuration succeded.");
    return TRUE;
}

swapchain_configuration    vc_swapchain_configuration_get(vc_ctx   *ctx)
{
    return ctx->swapchain_conf;
}


