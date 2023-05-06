#include "vulcain.h"

// All "boilerplate" objects : instance, device, queues, so on

static VkResult vc_vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if(func)
    {
        return func(instance, pCreateInfo, pAllocator, pMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void vc_vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if(func)
    {
        func(instance, messenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vc_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
{
    switch(messageSeverity)
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

    //Create instance
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

    if(desc->enable_debugging)
    {
        //Concatenate requested exts to debug ext
        inst_ci.ppEnabledExtensionNames = mem_allocate(sizeof(char*) * inst_ci.enabledExtensionCount, MEMORY_TAG_DARRAY);

        for(int i = 0; i < desc->extension_count; i++)
        {
            *((char **)&inst_ci.ppEnabledExtensionNames[i + 1]) = desc->extensions[i];
        }
        *((char **)&inst_ci.ppEnabledExtensionNames[0]) = debug_ext;
    }
    else
    {
        inst_ci.ppEnabledExtensionNames = (const char * const*)desc->extensions;
    }

    char *layers[1] = {"VK_LAYER_KHRONOS_validation"};
    inst_ci.ppEnabledLayerNames = (const char * const *)layers;

    //Check for extension availability
    {
        u32 count = 0;
        vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
        VkExtensionProperties *props = mem_allocate(sizeof(VkExtensionProperties) * count, MEMORY_TAG_RENDERER);
        vkEnumerateInstanceExtensionProperties(NULL, &count, props);

        for(int i = 0; i < inst_ci.enabledExtensionCount; i++)
        {
            b8 found = FALSE;
            for(int j = 0; j < count; j++)
            {
                if(strcmp(props[j].extensionName, inst_ci.ppEnabledExtensionNames[i]) == 0)
                {
                    found = TRUE;
                }
            }
            if(!found) {FATAL("Extension '%s' not supported by instance.", inst_ci.ppEnabledExtensionNames[i]); return FALSE;}
        }
    }

    //Check for layers availability
    {
        u32 count = 0;
        vkEnumerateInstanceLayerProperties(&count, NULL);
        VkLayerProperties *props = mem_allocate(sizeof(VkLayerProperties) * count, MEMORY_TAG_RENDERER);
        vkEnumerateInstanceLayerProperties(&count, props);

        for(int i = 0; i < inst_ci.enabledLayerCount; i++)
        {
            b8 found = FALSE;
            for(int j = 0; j < count; j++)
            {
                if(strcmp(props[j].layerName, inst_ci.ppEnabledLayerNames[i]) == 0)
                {
                    found = TRUE;
                }
            }
            if(!found) {FATAL("Layer '%s' not supported by instance.", inst_ci.ppEnabledLayerNames[i]); return FALSE;}
        }
    }
    TRACE("Extensions and layers all supported.");

    VK_CHECKR(vkCreateInstance(&inst_ci, NULL, &ctx->vk_instance), "Could not create instance");

    ctx->vk_debug_messenger = VK_NULL_HANDLE;
    if(desc->enable_debugging)
    {
        mem_free((void*)inst_ci.ppEnabledExtensionNames);

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
    u32 physical_device_count = 0;
    vkEnumeratePhysicalDevices(ctx->vk_instance, &physical_device_count, NULL);
    VkPhysicalDevice *physical_devices = mem_allocate(sizeof(VkPhysicalDevice) * physical_device_count, MEMORY_TAG_RENDERER);
    vkEnumeratePhysicalDevices(ctx->vk_instance, &physical_device_count, physical_devices);

    INFO("%d physical devices detected.", physical_device_count);
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    for(int i = 0; i < physical_device_count; i++)
    {
        if(_vc_priv_is_physical_device_suitable(ctx, query, physical_devices[i], ctx->vk_window_surface))
        {
            physical_device = physical_devices[i];
        }
    }

    if(!physical_device)
    {
        ERROR("No device respecting query found.");
        return FALSE;
    }

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physical_device, &props);

    INFO("Selecting device : '%s'", props.deviceName);

    ctx->vk_selected_physical_device = physical_device;
    return TRUE;
}

b8 _vc_priv_is_physical_device_suitable(vc_ctx *ctx, physical_device_query query, VkPhysicalDevice phys_device, VkSurfaceKHR surface)
{
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(phys_device, &props);

    if((query.allowed_types & props.deviceType) == 0)
    {
        return FALSE;
    }

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(phys_device, &features);

    /* ---------------- Features ---------------- */
    if(query.requested_features.geometryShader && !features.geometryShader) return FALSE;

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, NULL);
    VkQueueFamilyProperties *queue_properties = mem_allocate(sizeof(VkQueueFamilyProperties) * queue_family_count, MEMORY_TAG_RENDERER);
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, queue_properties);

    b8 has_graphics = FALSE;
    b8 has_compute = FALSE;
    b8 has_present = FALSE;

    if(query.supports_present & !surface)
    {
        ERROR("A window surface is necessary when a present queue is requested in query.");
    }

    for(int i = 0; i < queue_family_count; i++)
    {
        if(queue_properties->queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            has_graphics = TRUE;
            has_compute = TRUE;
        }

        if(queue_properties->queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            has_compute = TRUE;
        }

        if(surface != VK_NULL_HANDLE)
        {
            VkBool32 present_support = FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(phys_device, i, surface, &present_support);
            if(present_support)
            {
                has_present = TRUE;
            }
        }
    }

    mem_free(queue_properties);

    if(query.supports_graphics && !has_graphics) return FALSE;
    if(query.supports_compute && !has_compute) return FALSE;
    if(query.supports_present && !has_present) return FALSE;

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

    if(ctx->vk_window_surface)
    {
        vkDestroySurfaceKHR(ctx->vk_instance, ctx->vk_window_surface, NULL);
    }

    if(ctx->vk_debug_messenger)
    {
        vc_vkDestroyDebugUtilsMessengerEXT(ctx->vk_instance, ctx->vk_debug_messenger, NULL);
    }

    vkDestroyInstance(ctx->vk_instance, NULL);
}