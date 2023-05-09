#include "vulcain.h"

static const char *const VC_EXT_VK_KHR_SWAPCHAIN_name = "VK_KHR_swapchain";

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

b8 vc_create_ctx(vc_ctx *ctx, instance_desc *desc)
{
    ctx->vk_window_surface = VK_NULL_HANDLE;

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

b8 vc_select_create_device(vc_ctx *ctx, physical_device_query query)
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
    if(query.request_main_queue)
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
        darray_push(device_extensions, (char*)VC_EXT_VK_KHR_SWAPCHAIN_name);
    }

    if(query.request_compute_queue)
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

    if(query.request_transfer_queue)
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

    if(query.request_main_queue)
    {
        vkGetDeviceQueue(ctx->vk_device, ctx->queues.indices[VC_QUEUE_MAIN], 0, &ctx->queues.queues[VC_QUEUE_MAIN]);
    }

    if(query.request_compute_queue)
    {
        vkGetDeviceQueue(ctx->vk_device, ctx->queues.indices[VC_QUEUE_COMPUTE], 0, &ctx->queues.queues[VC_QUEUE_COMPUTE]);
    }

    if(query.request_transfer_queue)
    {
        vkGetDeviceQueue(ctx->vk_device, ctx->queues.indices[VC_QUEUE_TRANSFER], 0, &ctx->queues.queues[VC_QUEUE_TRANSFER]);
    }

    TRACE("Retrieved queues.");

    return TRUE;
}

b8 _vc_priv_select_swapchain_configuration(vc_ctx *ctx)
{
    // Select swapchain format
    u32 format_count = 0;
    VkSurfaceFormatKHR *formats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &format_count, NULL);
    formats = mem_allocate(format_count * sizeof(VkSurfaceFormatKHR), MEMORY_TAG_RENDERER);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &format_count, formats);

    ctx->swapchain_conf.swapchain_format = formats[0]; // Fallback
    for(int i = 0; i < format_count; i++)
    {
        if((formats[i].format == VK_FORMAT_R8G8B8A8_SRGB || formats[i].format == VK_FORMAT_R8G8B8A8_SNORM) && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            ctx->swapchain_conf.swapchain_format = formats[i];
        }
    }
    mem_free(formats);

    // Select present mode

    u32 present_mode_count = 0;
    VkPresentModeKHR *present_modes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &present_mode_count, NULL);
    present_modes = mem_allocate(sizeof(VkPresentModeKHR) * present_mode_count, MEMORY_TAG_RENDERER);
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &present_mode_count, present_modes);

    ctx->swapchain_conf.present_mode = VK_PRESENT_MODE_FIFO_KHR; // FIFO Is always available
    for(int i = 0; i < present_mode_count; i++)
    {
        if(present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            ctx->swapchain_conf.present_mode = present_modes[i];
            INFO("Got mailbox present mode.");
        }
    }
    mem_free(present_modes);

    VkSurfaceCapabilitiesKHR capa;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->vk_selected_physical_device, ctx->vk_window_surface, &capa);

    ctx->swapchain_conf.image_count = capa.minImageCount + 1;
    
    if(capa.maxImageCount != 0) // If not infinite available images, cap
    {
        ctx->swapchain_conf.image_count= CLAMP(ctx->swapchain_conf.image_count, capa.minImageCount, capa.maxImageCount);
    }
    ctx->swapchain_conf.capabilities = capa;

    TRACE("Using %d swapchain images.", ctx->swapchain_conf.image_count);

    // Depth buffer format
    u32 candidates_count = 3;
    VkFormat depth_candidates[3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    b8 found = FALSE;
    for(int i = 0; i < candidates_count; i++)
    {
        VkFormatProperties props = {0};
        vkGetPhysicalDeviceFormatProperties(ctx->vk_selected_physical_device, depth_candidates[i], &props);

        if((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            ctx->swapchain_conf.depth_format = depth_candidates[i];
            found = TRUE;
        }
    }

    if(!found)
    {
        ERROR("No possible depth format found.");
    }

    TRACE("Depth format: %d", ctx->swapchain_conf.depth_format);

    return TRUE;
}

i32 _vc_priv_search_main_queue(VkQueueFamilyProperties *props, u32 prop_count)
{
    for(int i = 0; i < prop_count; i ++)
    {
        if(props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            return i;
        }
    }
    return -1;
}

i32 _vc_priv_search_dedicated_compute_queue(VkQueueFamilyProperties *props, u32 prop_count)
{
    //Searches a dedicated compute
    for(int i = 0; i < prop_count; i ++)
    {
        if((props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
        {
            return i;
        }
    }

    return -1;
}

i32 _vc_priv_search_dedicated_transfer_queue(VkQueueFamilyProperties *props, u32 prop_count)
{
    //Searches a dedicated transfer queue
    for(int i = 0; i < prop_count; i ++)
    {
        if((props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
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

b8 vc_get_surface_glfw(vc_ctx *ctx, GLFWwindow *window)
{
    VK_CHECKR(glfwCreateWindowSurface(ctx->vk_instance, window, NULL, &ctx->vk_window_surface), "Could not create GLFW window surface.");
    return TRUE;
}

void vc_destroy_ctx(vc_ctx *ctx)
{
    TRACE("Destroying vc context.");

    if (ctx->vk_device)
    {
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