#include "../base/data_structures/hashmap.h"
#include "vc_handles.h"
#include "vc_managed_types.h"
#include "vulcain.h"
#include <vulkan/vulkan_core.h>

#include "vc_private.h"

//TODO: Cut this file into smaller parts

static const char * const VC_EXT_VK_KHR_SWAPCHAIN_name = "VK_KHR_swapchain";

b8                                       _vc_priv_setup_instance(vc_ctx *ctx, instance_desc *desc);
b8                                       _vc_priv_select_create_device(vc_ctx *ctx, physical_device_query query);
b8                                       _vc_priv_is_physical_device_suitable(vc_ctx *ctx, physical_device_query query, VkPhysicalDevice phys_device, VkSurfaceKHR surface);
i32                                      _vc_priv_search_physical_device_queue(vc_ctx *ctx, vc_queue_type type, VkPhysicalDevice phys_device, VkSurfaceKHR surface);

// All "boilerplate" objects : instance, device, queues, so on

static VkResult                          vc_vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func)
    {
        return func(instance, pCreateInfo, pAllocator, pMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void                              vc_vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func)
    {
        func(instance, messenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL    vc_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
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

// Identity hash function
u64    _vc_priv_u64_hash_id(void *obj, u64 size)
{
    return *(u64 *)obj;
}

b8     vc_create_ctx(vc_ctx *ctx, instance_desc *desc, physical_device_query *phys_device_query)
{
    hashmap_create(&ctx->desc_set_layouts_hashmap, 17, sizeof(u64), sizeof(vc_descriptor_set_layout), _vc_priv_u64_hash_id);

    // TODO: Make this configurable, or allow handle pools to be resizable (need to modify the base layer for that)
    vc_handle_mgr_create(
        &ctx->handle_manager,
        (vc_handle_mgr_counts){
            [VC_HANDLE_COMPUTE_PIPE]          = 64,
            [VC_HANDLE_GRAPHICS_PIPE]         = 32,
            [VC_HANDLE_COMMAND_BUFFER]        = 16,
            [VC_HANDLE_SEMAPHORE]             = 32,
            [VC_HANDLE_IMAGE]                 = 64,
            [VC_HANDLE_IMAGE_VIEW]            = 64,
            [VC_HANDLE_IMAGE_SAMPLER]         = 64,
            [VC_HANDLE_BUFFER]                = 64,
            [VC_HANDLE_DESCRIPTOR_SET_LAYOUT] = 64,
            [VC_HANDLE_DESCRIPTOR_SET]        = 128,
            [VC_HANDLE_RENDER_PASS]           = 16,
            [VC_HANDLE_FRAMEBUFFER]           = 32,
        }
        );

    if ( !_vc_priv_setup_instance(ctx, desc) )
    {
        FATAL("Could not setup vkInstance, aborting.");
        return FALSE;
    }
    ctx->vk_window_surface = VK_NULL_HANDLE;
    ctx->use_windowing     = desc->enable_windowing;
    ctx->windowing_system  = desc->windowing_system;

    if (ctx->use_windowing)
    {
        VK_CHECKR(ctx->windowing_system.get_window_surface_fun(ctx->windowing_system.windowing_ctx, ctx->vk_instance, &ctx->vk_window_surface), "Could not create window surface.");
    }
    if ( !_vc_priv_select_create_device(ctx, *phys_device_query) )
    {
        FATAL("Could not setup device, queues, command pools or swapchain, aborting.");
        return FALSE;
    }
    INFO("Vulcain instance setup.");

    INFO("Creating descriptor pool");

    // Descriptor set allocator creation
    {
        vc_priv_descriptor_set_allocator_create(ctx, &ctx->set_allocator);
    }

    // Vma creation
    {
        VmaVulkanFunctions vma_vulkan_func =
        {
            0
        };
        vma_vulkan_func.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vma_vulkan_func.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocator_ci =
        {
            0
        };
        allocator_ci.vulkanApiVersion = VK_API_VERSION_1_0;
        allocator_ci.physicalDevice   = ctx->vk_selected_physical_device;
        allocator_ci.device           = ctx->vk_device;
        allocator_ci.instance         = ctx->vk_instance;
        allocator_ci.pVulkanFunctions = &vma_vulkan_func;

        VK_CHECKR(vmaCreateAllocator(&allocator_ci, &ctx->vma_allocator), "Could not create a VMA Allocator.");
    }

    return TRUE;
}

b8    _vc_priv_setup_instance(vc_ctx *ctx, instance_desc *desc)
{
    // Create instance
    VkApplicationInfo app_info =
    {
        VK_STRUCTURE_TYPE_APPLICATION_INFO
    };
    app_info.pApplicationName   = desc->app_name;
    app_info.pEngineName        = desc->engine_name;
    app_info.applicationVersion = desc->app_version;
    app_info.apiVersion         = VK_API_VERSION_1_2;
    app_info.engineVersion      = desc->engine_version;

    VkInstanceCreateInfo inst_ci =
    {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
    };
    inst_ci.pApplicationInfo      = &app_info;
    inst_ci.enabledExtensionCount = desc->extension_count + (desc->enable_debugging ? 1 : 0);
    inst_ci.enabledLayerCount     = desc->enable_debugging ? 1 : 0;

    char *debug_ext = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    if (desc->enable_debugging)
    {
        // Concatenate requested exts to debug ext
        inst_ci.ppEnabledExtensionNames = mem_allocate(sizeof(char *) * inst_ci.enabledExtensionCount, MEMORY_TAG_DARRAY);

        for (int i = 0; i < desc->extension_count; i++)
        {
            *( (char **)&inst_ci.ppEnabledExtensionNames[i + 1] ) = desc->extensions[i];
        }
        *( (char **)&inst_ci.ppEnabledExtensionNames[0] ) = debug_ext;
    }
    else
    {
        inst_ci.ppEnabledExtensionNames = (const char * const *)desc->extensions;
    }
    char *layers[1] =
    {
        "VK_LAYER_KHRONOS_validation"
    };
    inst_ci.ppEnabledLayerNames = (const char * const *)layers;

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
    ctx->debug_enabled      = desc->enable_debugging;
    if (desc->enable_debugging)
    {
        mem_free( (void *)inst_ci.ppEnabledExtensionNames );

        VkDebugUtilsMessengerCreateInfoEXT dbg_info =
        {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT
        };
        dbg_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dbg_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbg_info.pfnUserCallback = vc_debug_callback;
        dbg_info.pUserData       = NULL;

        VK_CHECKR(vc_vkCreateDebugUtilsMessengerEXT(ctx->vk_instance, &dbg_info, NULL, &ctx->vk_debug_messenger), "Debug messenger could not be created");
    }
    return TRUE;
}

b8    _vc_priv_select_create_device(vc_ctx *ctx, physical_device_query query)
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
        if ( _vc_priv_is_physical_device_suitable(ctx, query, physical_devices[i], ctx->vk_window_surface) )
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
    char **device_extensions           = darray_create(char *);

    // Search queues
    if (query.request_main_queue)
    {
        VkDeviceQueueCreateInfo queue_ci =
        {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueCount       = 1,
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
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueCount       = 1,
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
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueCount       = 1,
            .pQueuePriorities = &ctx->queues.priorities[VC_QUEUE_TRANSFER],
            .queueFamilyIndex = _vc_priv_search_physical_device_queue(ctx, VC_QUEUE_TRANSFER, ctx->vk_selected_physical_device, ctx->vk_window_surface),
        };

        ctx->queues.indices[VC_QUEUE_TRANSFER] = queue_ci.queueFamilyIndex;

        darray_push(queues_ci, queue_ci);
    }
    VkDeviceCreateInfo device_ci =
    {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount    = darray_length(queues_ci),
        .pQueueCreateInfos       = queues_ci,
        .enabledLayerCount       = 0,
        .enabledExtensionCount   = darray_length(device_extensions),
        .ppEnabledExtensionNames = (const char * const *)device_extensions,
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
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = ctx->queues.indices[VC_QUEUE_MAIN]
        };

        VK_CHECKR(vkCreateCommandPool(ctx->vk_device, &pool_ci, NULL, &ctx->queues.pools[VC_QUEUE_MAIN]), "Could not create command pool.");
    }
    if (query.request_compute_queue)
    {
        vkGetDeviceQueue(ctx->vk_device, ctx->queues.indices[VC_QUEUE_COMPUTE], 0, &ctx->queues.queues[VC_QUEUE_COMPUTE]);

        VkCommandPoolCreateInfo pool_ci =
        {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = ctx->queues.indices[VC_QUEUE_COMPUTE]
        };

        VK_CHECKR(vkCreateCommandPool(ctx->vk_device, &pool_ci, NULL, &ctx->queues.pools[VC_QUEUE_COMPUTE]), "Could not create command pool.");
    }
    if (query.request_transfer_queue)
    {
        vkGetDeviceQueue(ctx->vk_device, ctx->queues.indices[VC_QUEUE_TRANSFER], 0, &ctx->queues.queues[VC_QUEUE_TRANSFER]);

        VkCommandPoolCreateInfo pool_ci =
        {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = ctx->queues.indices[VC_QUEUE_TRANSFER]
        };

        VK_CHECKR(vkCreateCommandPool(ctx->vk_device, &pool_ci, NULL, &ctx->queues.pools[VC_QUEUE_TRANSFER]), "Could not create command pool.");
    }
    TRACE("Retrieved queues.");

    if (ctx->use_windowing)
    {
        INFO("Using windowing, swapchain creation is allowed");
    }
    return TRUE;
}

i32    _vc_priv_search_main_queue(VkQueueFamilyProperties *props, u32 prop_count)
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

i32    _vc_priv_search_dedicated_compute_queue(VkQueueFamilyProperties *props, u32 prop_count)
{
    // Searches a dedicated compute
    for (int i = 0; i < prop_count; i++)
    {
        if ( (props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && ( (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 ) )
        {
            return i;
        }
    }
    return -1;
}

i32    _vc_priv_search_dedicated_transfer_queue(VkQueueFamilyProperties *props, u32 prop_count)
{
    // Searches a dedicated transfer queue
    for (int i = 0; i < prop_count; i++)
    {
        if ( (props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && ( (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 ) )
        {
            return i;
        }
    }
    return -1;
}

i32    _vc_priv_search_physical_device_queue(vc_ctx *ctx, vc_queue_type type, VkPhysicalDevice phys_device, VkSurfaceKHR surface)
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

b8    _vc_priv_is_physical_device_suitable(vc_ctx *ctx, physical_device_query query, VkPhysicalDevice phys_device, VkSurfaceKHR surface)
{
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(phys_device, &props);

    if ( (query.allowed_types & props.deviceType) == 0 )
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

void    vc_destroy_ctx(vc_ctx   *ctx)
{
    vkDeviceWaitIdle(ctx->vk_device);
    TRACE("Destroying vc context.");

    // Check for swapchain existence is handled
    vc_swapchain_cleanup(ctx);

    vc_handle_mgr_destroy(&ctx->handle_manager, ctx);
    hashmap_destroy(&ctx->desc_set_layouts_hashmap);

    vmaDestroyAllocator(ctx->vma_allocator);

    // Descriptor pool destruction
    {
        vc_priv_descriptor_set_allocator_destroy(ctx, &ctx->set_allocator);
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

void          vc_handle_destroy(vc_ctx *ctx, vc_handle hndl)
{
    vc_handle_mgr_free(&ctx->handle_manager, hndl, ctx);
}

void          vc_queue_wait_idle(vc_ctx *ctx, vc_queue_type type)
{
    vkQueueWaitIdle(ctx->queues.queues[type]);
}



inline u32    vc_u32_flags_set_bits(u32    flag)
{
    u32 count = 0;
    do
    {
        count += flag & 1;
    }
    while(flag >>= 1);
    return count;
}

void    vc_queue_flags_to_queue_indices_list(vc_ctx *ctx, vc_queue_flags flags, u32 *ids)
{
    /*TODO: Find out why I needed this*/ }
