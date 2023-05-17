#include "vc_handles.h"
#include "vc_managed_types.h"
#include "vulcain.h"
#include <vulkan/vulkan_core.h>

static const char *const VC_EXT_VK_KHR_SWAPCHAIN_name = "VK_KHR_swapchain";

b8  _vc_priv_setup_default_swapchain(vc_ctx *ctx);
b8  _vc_priv_setup_instance(vc_ctx *ctx, instance_desc *desc);
b8  _vc_priv_select_create_device(vc_ctx *ctx, physical_device_query query);
b8  _vc_priv_is_physical_device_suitable(vc_ctx *ctx, physical_device_query query, VkPhysicalDevice phys_device, VkSurfaceKHR surface);
i32 _vc_priv_search_physical_device_queue(vc_ctx *ctx, vc_queue_type type, VkPhysicalDevice phys_device, VkSurfaceKHR surface);

// All "boilerplate" objects : instance, device, queues, so on

static VkResult vc_vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func)
    {
        return func(instance, pCreateInfo, pAllocator, pMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void vc_vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func)
    {
        func(instance, messenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vc_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        TRACE("[VULKAN]: %s", pCallbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        INFO("[VULKAN]: %s", pCallbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        WARN("[VULKAN]: %s", pCallbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        ERROR("[VULKAN]: %s", pCallbackData->pMessage);
        break;

    default:
        TRACE("[VULKAN]: %s", pCallbackData->pMessage);
        break;
    }
    return VK_FALSE;
}

b8 vc_create_ctx(vc_ctx *ctx, instance_desc *desc, physical_device_query *phys_device_query)
{
    if (!_vc_priv_setup_instance(ctx, desc))
    {
        FATAL("Could not setup vkInstance, aborting.");
        return FALSE;
    }

    ctx->vk_window_surface = VK_NULL_HANDLE;
    ctx->use_windowing = desc->enable_windowing;
    ctx->windowing_system = desc->windowing_system;

    if (ctx->use_windowing)
    {
        VK_CHECKR(ctx->windowing_system.get_window_surface_fun(ctx->windowing_system.windowing_ctx, ctx->vk_instance, &ctx->vk_window_surface), "Could not create window surface.");
    }

    if (!_vc_priv_select_create_device(ctx, *phys_device_query))
    {
        FATAL("Could not setup device, queues, command pools or swapchain, aborting.");
        return FALSE;
    }
    INFO("Vulcain instance setup.");

    // TODO: Make this configurable
    vc_handle_mgr_create(&ctx->handle_manager, (vc_handle_mgr_counts){
                                                   [VC_HANDLE_COMPUTE_PIPE] = 64,
                                                   [VC_HANDLE_COMMAND_BUFFER] = 16,
                                                   [VC_HANDLE_SEMAPHORE] = 32,
                                               });

    return TRUE;
}

b8 _vc_priv_setup_instance(vc_ctx *ctx, instance_desc *desc)
{
    // Create instance
    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.pApplicationName = desc->app_name;
    app_info.pEngineName = desc->engine_name;
    app_info.applicationVersion = desc->app_version;
    app_info.apiVersion = VK_API_VERSION_1_2;
    app_info.engineVersion = desc->engine_version;

    VkInstanceCreateInfo inst_ci = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    inst_ci.pApplicationInfo = &app_info;
    inst_ci.enabledExtensionCount = desc->extension_count + (desc->enable_debugging ? 1 : 0);
    inst_ci.enabledLayerCount = desc->enable_debugging ? 1 : 0;

    char *debug_ext = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    if (desc->enable_debugging)
    {
        // Concatenate requested exts to debug ext
        inst_ci.ppEnabledExtensionNames = mem_allocate(sizeof(char *) * inst_ci.enabledExtensionCount, MEMORY_TAG_DARRAY);

        for (int i = 0; i < desc->extension_count; i++)
        {
            *((char **)&inst_ci.ppEnabledExtensionNames[i + 1]) = desc->extensions[i];
        }
        *((char **)&inst_ci.ppEnabledExtensionNames[0]) = debug_ext;
    }
    else
    {
        inst_ci.ppEnabledExtensionNames = (const char *const *)desc->extensions;
    }

    char *layers[1] = {"VK_LAYER_KHRONOS_validation"};
    inst_ci.ppEnabledLayerNames = (const char *const *)layers;

    // Check for extension availability
    {
        u32 count = 0;
        vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
        VkExtensionProperties *props = mem_allocate(sizeof(VkExtensionProperties) * count, MEMORY_TAG_RENDERER);
        vkEnumerateInstanceExtensionProperties(NULL, &count, props);

        for (int i = 0; i < inst_ci.enabledExtensionCount; i++)
        {
            b8 found = FALSE;
            for (int j = 0; j < count; j++)
            {
                if (strcmp(props[j].extensionName, inst_ci.ppEnabledExtensionNames[i]) == 0)
                {
                    found = TRUE;
                }
            }
            if (!found)
            {
                FATAL("Extension '%s' not supported by instance.", inst_ci.ppEnabledExtensionNames[i]);
                return FALSE;
            }
        }
    }

    // Check for layers availability
    {
        u32 count = 0;
        vkEnumerateInstanceLayerProperties(&count, NULL);
        VkLayerProperties *props = mem_allocate(sizeof(VkLayerProperties) * count, MEMORY_TAG_RENDERER);
        vkEnumerateInstanceLayerProperties(&count, props);

        for (int i = 0; i < inst_ci.enabledLayerCount; i++)
        {
            b8 found = FALSE;
            for (int j = 0; j < count; j++)
            {
                if (strcmp(props[j].layerName, inst_ci.ppEnabledLayerNames[i]) == 0)
                {
                    found = TRUE;
                }
            }
            if (!found)
            {
                FATAL("Layer '%s' not supported by instance.", inst_ci.ppEnabledLayerNames[i]);
                return FALSE;
            }
        }
    }
    TRACE("Extensions and layers all supported.");

    VK_CHECKR(vkCreateInstance(&inst_ci, NULL, &ctx->vk_instance), "Could not create instance");

    ctx->vk_debug_messenger = VK_NULL_HANDLE;
    if (desc->enable_debugging)
    {
        mem_free((void *)inst_ci.ppEnabledExtensionNames);

        VkDebugUtilsMessengerCreateInfoEXT dbg_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        dbg_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dbg_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbg_info.pfnUserCallback = vc_debug_callback;
        dbg_info.pUserData = NULL;

        VK_CHECKR(vc_vkCreateDebugUtilsMessengerEXT(ctx->vk_instance, &dbg_info, NULL, &ctx->vk_debug_messenger), "Debug messenger could not be created");
    }

    return TRUE;
}

b8 _vc_priv_select_create_device(vc_ctx *ctx, physical_device_query query)
{
    // Search suitable physical devices
    u32 physical_device_count = 0;
    vkEnumeratePhysicalDevices(ctx->vk_instance, &physical_device_count, NULL);
    VkPhysicalDevice *physical_devices = mem_allocate(sizeof(VkPhysicalDevice) * physical_device_count, MEMORY_TAG_RENDERER);
    vkEnumeratePhysicalDevices(ctx->vk_instance, &physical_device_count, physical_devices);

    INFO("%d physical devices detected.", physical_device_count);
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    for (int i = 0; i < physical_device_count; i++)
    {
        if (_vc_priv_is_physical_device_suitable(ctx, query, physical_devices[i], ctx->vk_window_surface))
        {
            physical_device = physical_devices[i];
        }
    }

    if (!physical_device)
    {
        ERROR("No device respecting query found.");
        return FALSE;
    }

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physical_device, &props);

    INFO("Selecting device : '%s'", props.deviceName);

    ctx->vk_selected_physical_device = physical_device;

    TRACE("Creating device");

    VkDeviceQueueCreateInfo *queues_ci = darray_create(VkDeviceQueueCreateInfo);
    char                   **device_extensions = darray_create(char *);

    // Search queues
    if (query.request_main_queue)
    {
        VkDeviceQueueCreateInfo queue_ci =
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueCount = 1,
                .pQueuePriorities = &ctx->queues.priorities[VC_QUEUE_MAIN],
                .queueFamilyIndex = _vc_priv_search_physical_device_queue(ctx, VC_QUEUE_MAIN, ctx->vk_selected_physical_device, ctx->vk_window_surface),
            };

        ctx->queues.indices[VC_QUEUE_MAIN] = queue_ci.queueFamilyIndex;

        darray_push(queues_ci, queue_ci);
        darray_push(device_extensions, (char *)VC_EXT_VK_KHR_SWAPCHAIN_name);
    }

    if (query.request_compute_queue)
    {
        VkDeviceQueueCreateInfo queue_ci =
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueCount = 1,
                .pQueuePriorities = &ctx->queues.priorities[VC_QUEUE_COMPUTE],
                .queueFamilyIndex = _vc_priv_search_physical_device_queue(ctx, VC_QUEUE_COMPUTE, ctx->vk_selected_physical_device, ctx->vk_window_surface),
            };

        ctx->queues.indices[VC_QUEUE_COMPUTE] = queue_ci.queueFamilyIndex;

        darray_push(queues_ci, queue_ci);
    }

    if (query.request_transfer_queue)
    {
        VkDeviceQueueCreateInfo queue_ci =
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueCount = 1,
                .pQueuePriorities = &ctx->queues.priorities[VC_QUEUE_TRANSFER],
                .queueFamilyIndex = _vc_priv_search_physical_device_queue(ctx, VC_QUEUE_TRANSFER, ctx->vk_selected_physical_device, ctx->vk_window_surface),
            };

        ctx->queues.indices[VC_QUEUE_TRANSFER] = queue_ci.queueFamilyIndex;

        darray_push(queues_ci, queue_ci);
    }

    VkDeviceCreateInfo device_ci =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = darray_length(queues_ci),
            .pQueueCreateInfos = queues_ci,
            .enabledLayerCount = 0,
            .enabledExtensionCount = darray_length(device_extensions),
            .ppEnabledExtensionNames = (const char *const *)device_extensions,
        };

    VK_CHECKR(vkCreateDevice(ctx->vk_selected_physical_device, &device_ci, NULL, &ctx->vk_device), "Couldn't create logical device.");
    TRACE("Created device.");

    darray_destroy(queues_ci);
    darray_destroy(device_extensions);

    if (query.request_main_queue)
    {
        vkGetDeviceQueue(ctx->vk_device, ctx->queues.indices[VC_QUEUE_MAIN], 0, &ctx->queues.queues[VC_QUEUE_MAIN]);

        VkCommandPoolCreateInfo pool_ci =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = ctx->queues.indices[VC_QUEUE_MAIN]};

        VK_CHECKR(vkCreateCommandPool(ctx->vk_device, &pool_ci, NULL, &ctx->queues.pools[VC_QUEUE_MAIN]), "Could not create command pool.");
    }

    if (query.request_compute_queue)
    {
        vkGetDeviceQueue(ctx->vk_device, ctx->queues.indices[VC_QUEUE_COMPUTE], 0, &ctx->queues.queues[VC_QUEUE_COMPUTE]);

        VkCommandPoolCreateInfo pool_ci =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = ctx->queues.indices[VC_QUEUE_COMPUTE]};

        VK_CHECKR(vkCreateCommandPool(ctx->vk_device, &pool_ci, NULL, &ctx->queues.pools[VC_QUEUE_COMPUTE]), "Could not create command pool.");
    }

    if (query.request_transfer_queue)
    {
        vkGetDeviceQueue(ctx->vk_device, ctx->queues.indices[VC_QUEUE_TRANSFER], 0, &ctx->queues.queues[VC_QUEUE_TRANSFER]);

        VkCommandPoolCreateInfo pool_ci =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = ctx->queues.indices[VC_QUEUE_TRANSFER]};

        VK_CHECKR(vkCreateCommandPool(ctx->vk_device, &pool_ci, NULL, &ctx->queues.pools[VC_QUEUE_TRANSFER]), "Could not create command pool.");
    }

    TRACE("Retrieved queues.");

    if (ctx->use_windowing)
    {
        INFO("Using windowing, creating swapchain.");
        _vc_priv_setup_default_swapchain(ctx);
    }

    return TRUE;
}

b8 _vc_priv_create_swapchain(vc_ctx *ctx, VkExtent2D extent)
{
    VkSwapchainCreateInfoKHR sc_ci =
        {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = ctx->vk_window_surface,
            .minImageCount = ctx->swapchain_conf.image_count,
            .imageFormat = ctx->swapchain_conf.swapchain_format.format,
            .imageColorSpace = ctx->swapchain_conf.swapchain_format.colorSpace,
            .imageExtent = (VkExtent2D){.width = extent.width, .height = extent.height},
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .preTransform = ctx->swapchain_conf.capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .clipped = VK_FALSE,
    };

    ctx->swapchain_conf.swapchain_extent = extent;

    VK_CHECKR(vkCreateSwapchainKHR(ctx->vk_device, &sc_ci, NULL, &ctx->swapchain.vk_swapchain), "Could not create swapchain.");
    TRACE("Created swapchain successfully.");

    vkGetSwapchainImagesKHR(ctx->vk_device, ctx->swapchain.vk_swapchain, &ctx->swapchain.swapchain_image_count, NULL);
    ctx->swapchain.swapchain_image_views = mem_allocate(sizeof(VkImageView) * ctx->swapchain.swapchain_image_count, MEMORY_TAG_RENDERER);
    ctx->swapchain.swapchain_images = mem_allocate(sizeof(VkImage) * ctx->swapchain.swapchain_image_count, MEMORY_TAG_RENDERER);
    vkGetSwapchainImagesKHR(ctx->vk_device, ctx->swapchain.vk_swapchain, &ctx->swapchain.swapchain_image_count, ctx->swapchain.swapchain_images);
    TRACE("Retrieved swapchain images.");

    for (int i = 0; i < ctx->swapchain.swapchain_image_count; i++)
    {
        VkImageViewCreateInfo view_ci =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = ctx->swapchain.swapchain_images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = ctx->swapchain_conf.swapchain_format.format,
                .components =
                    {
                                 .a = VK_COMPONENT_SWIZZLE_A,
                                 .r = VK_COMPONENT_SWIZZLE_R,
                                 .g = VK_COMPONENT_SWIZZLE_G,
                                 .b = VK_COMPONENT_SWIZZLE_B,
                                 },
                .subresourceRange =
                    {
                                 .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                 .baseArrayLayer = 0,
                                 .baseMipLevel = 0,
                                 .layerCount = 1,
                                 .levelCount = 1,
                                 },
        };

        VK_CHECKR(vkCreateImageView(ctx->vk_device, &view_ci, NULL, &ctx->swapchain.swapchain_image_views[i]), "Could not create an image view.");
    }
    TRACE("Created swapchain image views.");
    return TRUE;
}

b8 _vc_priv_delete_swapchain(vc_ctx *ctx)
{
    for (int i = 0; i < ctx->swapchain.swapchain_image_count; i++)
    {
        vkDestroyImageView(ctx->vk_device, ctx->swapchain.swapchain_image_views[i], NULL);
    }
    vkDestroySwapchainKHR(ctx->vk_device, ctx->swapchain.vk_swapchain, NULL);

    mem_free(ctx->swapchain.swapchain_image_views);
    mem_free(ctx->swapchain.swapchain_images);

    return TRUE;
}

b8 _vc_priv_select_swapchain_configuration(vc_ctx *ctx)
{
    // Select swapchain format
    u32                 format_count = 0;
    VkSurfaceFormatKHR *formats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &format_count, NULL);
    formats = mem_allocate(format_count * sizeof(VkSurfaceFormatKHR), MEMORY_TAG_RENDERER);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &format_count, formats);

    /* ---------------- Swapchain format ---------------- */
    ctx->swapchain_conf.swapchain_format = formats[0]; // Fallback
    for (int i = 0; i < format_count; i++)
    {
        if ((formats[i].format == VK_FORMAT_R8G8B8A8_SRGB || formats[i].format == VK_FORMAT_R8G8B8A8_SNORM) && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            ctx->swapchain_conf.swapchain_format = formats[i];
        }
    }
    mem_free(formats);

    /* ---------------- Present mode ---------------- */
    u32               present_mode_count = 0;
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
    u32      candidates_count = 3;
    VkFormat depth_candidates[3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    b8       found = FALSE;
    for (int i = 0; i < candidates_count; i++)
    {
        VkFormatProperties props = {0};
        vkGetPhysicalDeviceFormatProperties(ctx->vk_selected_physical_device, depth_candidates[i], &props);

        if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            ctx->swapchain_conf.depth_format = depth_candidates[i];
            found = TRUE;
        }
    }

    if (!found)
    {
        WARN("No possible depth format found.");
    }

    TRACE("Depth format: %d", ctx->swapchain_conf.depth_format);

    return TRUE;
}

i32 _vc_priv_search_main_queue(VkQueueFamilyProperties *props, u32 prop_count)
{
    for (int i = 0; i < prop_count; i++)
    {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            return i;
        }
    }
    return -1;
}

i32 _vc_priv_search_dedicated_compute_queue(VkQueueFamilyProperties *props, u32 prop_count)
{
    // Searches a dedicated compute
    for (int i = 0; i < prop_count; i++)
    {
        if ((props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
        {
            return i;
        }
    }

    return -1;
}

i32 _vc_priv_search_dedicated_transfer_queue(VkQueueFamilyProperties *props, u32 prop_count)
{
    // Searches a dedicated transfer queue
    for (int i = 0; i < prop_count; i++)
    {
        if ((props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
        {
            return i;
        }
    }

    return -1;
}

i32 _vc_priv_search_physical_device_queue(vc_ctx *ctx, vc_queue_type type, VkPhysicalDevice phys_device, VkSurfaceKHR surface)
{
    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, NULL);
    VkQueueFamilyProperties *queue_properties = mem_allocate(sizeof(VkQueueFamilyProperties) * queue_family_count, MEMORY_TAG_RENDERER);
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, queue_properties);

    i32 queue_id = -1;
    switch (type)
    {
    case VC_QUEUE_MAIN:
        queue_id = _vc_priv_search_main_queue(queue_properties, queue_family_count);
        break;

    case VC_QUEUE_COMPUTE:
        queue_id = _vc_priv_search_dedicated_compute_queue(queue_properties, queue_family_count);
        break;

    case VC_QUEUE_TRANSFER:
        queue_id = _vc_priv_search_dedicated_transfer_queue(queue_properties, queue_family_count);
        break;

    default:
        ERROR("Invalid vc_queue_type enum.");
        break;
    }

    mem_free(queue_properties);
    return queue_id;
}

b8 _vc_priv_is_physical_device_suitable(vc_ctx *ctx, physical_device_query query, VkPhysicalDevice phys_device, VkSurfaceKHR surface)
{
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(phys_device, &props);

    if ((query.allowed_types & props.deviceType) == 0)
    {
        return FALSE;
    }

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(phys_device, &features);

    /* ---------------- Features ---------------- */
    if (query.requested_features.geometryShader && !features.geometryShader)
        return FALSE;

    if (query.request_main_queue && _vc_priv_search_physical_device_queue(ctx, VC_QUEUE_MAIN, phys_device, surface) < 0)
        return FALSE;
    if (query.request_compute_queue && _vc_priv_search_physical_device_queue(ctx, VC_QUEUE_COMPUTE, phys_device, surface) < 0)
        return FALSE;
    if (query.request_transfer_queue && _vc_priv_search_physical_device_queue(ctx, VC_QUEUE_TRANSFER, phys_device, surface) < 0)
        return FALSE;

    return TRUE;
}

b8 _vc_priv_get_optimal_swapchain_size(vc_ctx *ctx, VkExtent2D *extent)
{
    // Select extent
    u32 width = 0;
    u32 height = 0;
    ctx->windowing_system.get_framebuffer_size_fun(ctx->windowing_system.windowing_ctx, &width, &height);

    if (ctx->swapchain_conf.capabilities.maxImageExtent.width == U32_MAX)
    {
        extent->width = CLAMP(width, ctx->swapchain_conf.capabilities.minImageExtent.width, ctx->swapchain_conf.capabilities.maxImageExtent.width);
        extent->height = CLAMP(height, ctx->swapchain_conf.capabilities.minImageExtent.height, ctx->swapchain_conf.capabilities.maxImageExtent.height);
    }
    else
    {
        *extent = ctx->swapchain_conf.capabilities.currentExtent;
    }
    return TRUE;
}

b8 _vc_priv_setup_default_swapchain(vc_ctx *ctx)
{
    if (!ctx->use_windowing)
    {
        FATAL("Cannot setup a swapchain without a windowing system.");
        return FALSE;
    }
    INFO("Selecting swapchain configuration");

    _vc_priv_select_swapchain_configuration(ctx);
    VkExtent2D swp_extent;
    _vc_priv_get_optimal_swapchain_size(ctx, &swp_extent);
    _vc_priv_create_swapchain(ctx, swp_extent);
    return TRUE;
}

void vc_destroy_ctx(vc_ctx *ctx)
{
    vkDeviceWaitIdle(ctx->vk_device);
    TRACE("Destroying vc context.");

    vc_handle_mgr_destroy(&ctx->handle_manager, ctx);

    if (ctx->swapchain.vk_swapchain)
    {
        _vc_priv_delete_swapchain(ctx);
    }

    if (ctx->vk_device)
    {
        if (ctx->queues.pools[VC_QUEUE_MAIN])
        {
            vkDestroyCommandPool(ctx->vk_device, ctx->queues.pools[VC_QUEUE_MAIN], NULL);
        }

        if (ctx->queues.pools[VC_QUEUE_COMPUTE])
        {
            vkDestroyCommandPool(ctx->vk_device, ctx->queues.pools[VC_QUEUE_COMPUTE], NULL);
        }

        if (ctx->queues.pools[VC_QUEUE_TRANSFER])
        {
            vkDestroyCommandPool(ctx->vk_device, ctx->queues.pools[VC_QUEUE_TRANSFER], NULL);
        }

        vkDestroyDevice(ctx->vk_device, NULL);
    }

    if (ctx->vk_window_surface)
    {
        vkDestroySurfaceKHR(ctx->vk_instance, ctx->vk_window_surface, NULL);
    }

    if (ctx->vk_debug_messenger)
    {
        vc_vkDestroyDebugUtilsMessengerEXT(ctx->vk_instance, ctx->vk_debug_messenger, NULL);
    }

    vkDestroyInstance(ctx->vk_instance, NULL);
}

void vc_handle_destroy(vc_ctx *ctx, vc_handle hndl)
{
    vc_handle_mgr_free(&ctx->handle_manager, hndl, ctx);
}

b8 _vc_priv_command_buffer_destroy(vc_ctx *ctx, vc_priv_man_command_buffer *buffer)
{
    vkFreeCommandBuffers(ctx->vk_device, buffer->pool, 1, &buffer->command_buffer);
    return TRUE;
}

vc_command_buffer vc_command_buffer_main_create(vc_ctx *ctx, vc_queue_type queue)
{
    VkCommandBufferAllocateInfo alloc_ci =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandBufferCount = 1,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandPool = ctx->queues.pools[queue],
        };

    vc_priv_man_command_buffer buf;
    buf.queue_type = queue;
    buf.pool = alloc_ci.commandPool;
    VK_CHECKH(vkAllocateCommandBuffers(ctx->vk_device, &alloc_ci, &buf.command_buffer), "Could not allocate a command buffer in one of the main pools.");
    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_COMMAND_BUFFER, (vc_man_destroy_func)_vc_priv_command_buffer_destroy);
    return vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_COMMAND_BUFFER, &buf);
}

void vc_command_buffer_submit(vc_ctx *ctx, vc_command_buffer command_buffer)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);

    VkSubmitInfo submit_i =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &buf->command_buffer,
            .signalSemaphoreCount = 0,
            .waitSemaphoreCount = 0,
        };

    vkQueueSubmit(ctx->queues.queues[buf->queue_type], 1, &submit_i, VK_NULL_HANDLE);
}

void vc_command_buffer_begin(vc_ctx *ctx, vc_command_buffer command_buffer)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);

    VkCommandBufferBeginInfo begin_i =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

    vkBeginCommandBuffer(buf->command_buffer, &begin_i);
}

void vc_command_buffer_end(vc_ctx *ctx, vc_command_buffer command_buffer)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vkEndCommandBuffer(buf->command_buffer);
}

void vc_command_buffer_compute_pipeline(vc_ctx *ctx, vc_command_buffer command_buffer, compute_dispatch_desc *desc)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vc_priv_man_compute_pipe   *pipe = vc_handle_mgr_ptr(&ctx->handle_manager, desc->pipe);

    vkCmdBindPipeline(buf->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipe->pipeline);
    vkCmdDispatch(buf->command_buffer, desc->groups_x, desc->groups_y, desc->groups_z);
}

void vc_command_buffer_reset(vc_ctx *ctx, vc_command_buffer command_buffer)
{
    vc_priv_man_command_buffer *buf = vc_handle_mgr_ptr(&ctx->handle_manager, command_buffer);
    vkResetCommandBuffer(buf->command_buffer, 0);
}

void vc_queue_wait_idle(vc_ctx *ctx, vc_queue_type type)
{
    vkQueueWaitIdle(ctx->queues.queues[type]);
}

void vc_swapchain_acquire_image(vc_ctx *ctx, u32 *image_id, vc_semaphore acquired_semaphore)
{
    vc_priv_man_semaphore *sem = vc_handle_mgr_ptr(&ctx->handle_manager, acquired_semaphore);
    vkAcquireNextImageKHR(ctx->vk_device, ctx->swapchain.vk_swapchain, U64_MAX, sem->semaphore, VK_NULL_HANDLE, image_id);
}
