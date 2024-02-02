#include "vulcain.h"

#include <string.h>
#include <alloca.h>
#include "vc_enum_util.h"

#define FEMTOLOG_IMPLEMENTATION
#include "femtolog.h"

#include "base/base.h"
#include "base/data_structures/darray.h"

b8                             vc_priv_check_layers(char **layers, u32 count);
b8                             vc_priv_check_instance_extensions(char **extensions, u32 count);
VKAPI_ATTR VkBool32 VKAPI_CALL vc_priv_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

// "Dynamic" functions
static VkResult
vc_vkCreateDebugUtilsMessengerEXT(
    VkInstance                                  instance,
    const VkDebugUtilsMessengerCreateInfoEXT   *pCreateInfo,
    const VkAllocationCallbacks                *pAllocator,
    VkDebugUtilsMessengerEXT                   *pMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func)
    {
        return func(instance, pCreateInfo, pAllocator, pMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void
vc_vkDestroyDebugUtilsMessengerEXT(
    VkInstance                     instance,
    VkDebugUtilsMessengerEXT       messenger,
    const VkAllocationCallbacks   *pAllocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func)
    {
        func(instance, messenger, pAllocator);
    }
}

bool
vc_ctx_create(vc_ctx                *ctx,
              VkApplicationInfo      app_info,
              vc_windowing_system   *windowing_system,
              bool                   enable_debugging,
              uint32_t               layer_count,
              const char           **layer_names,
              uint32_t               extension_count,
              const char           **extension_names)
{
    vc_info("Creating vulcain context");

    ctx->debugging_enabled = enable_debugging;
    // Preparing windowing system
    ctx->windowing_enabled = FALSE;
    if(windowing_system)
    {
        vc_trace("Windowing system has been provided name='%s'", windowing_system->windowing_system_name);

        ctx->windowing_enabled = TRUE;
        ctx->windowing_system  = *windowing_system;
    }
    else
    {
        vc_trace("Windowing not enabled");
    }

    if(
        (layer_count != 0 && !layer_names) ||
        (extension_count != 0 && !extension_names)
        )
    {
        vc_fatal("Vulcain context creation error: Layer names or extension names not provided");
        return FALSE;
    }

    // Prepare layers :

    char **layers     = darray_create(char *);
    char **extensions = darray_create(char *);

    const char * const validation_name = "VK_LAYER_KHRONOS_validation";
    const char * const dbg_utils_name  = "VK_EXT_debug_utils";
    if(enable_debugging)
    {
        // In case of debugging
        darray_push(layers, (char *)validation_name);
        darray_push(extensions, (char *)dbg_utils_name);
    }

    // Add users extensions and layers
    {
        for(u32 i = 0; i < layer_count; i++)
        {
            darray_push(layers, layer_names[i]);
        }

        for(u32 i = 0; i < extension_count; i++)
        {
            darray_push(extensions, extension_names[i]);
        }
    }

    if( !vc_priv_check_layers( layers, darray_length(layers) ) )
    {
        vc_fatal("Some layers are not supported, aborting");
        return FALSE;
    }

    if( !vc_priv_check_instance_extensions( extensions, darray_length(extensions) ) )
    {
        vc_fatal("Some extensions are not supported, aborting");
        return FALSE;
    }

    vc_debug("Requested layers for instance :");
    for(u32 i = 0; i < darray_length(layers); i++)
    {
        vc_debug("\t%s", layers[i]);
    }

    vc_debug("Requested extensions for instance :");
    for(u32 i = 0; i < darray_length(extensions); i++)
    {
        vc_debug("\t%s", extensions[i]);
    }

    VkInstanceCreateInfo instance_ci =
    {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &app_info,
        .enabledLayerCount       = darray_length(layers),
        .enabledExtensionCount   = darray_length(extensions),
        .ppEnabledLayerNames     = (const char * const *)layers,
        .ppEnabledExtensionNames = (const char * const *)extensions,
    };

    VK_CHECKR(vkCreateInstance(&instance_ci, NULL, &ctx->vk_instance), "Could not create VkInstance");

    vc_info("Vulkan instance created.");

    // Debug utils creation
    if(enable_debugging)
    {
        VkDebugUtilsMessengerCreateInfoEXT debug_utils_ci =
        {
            .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pUserData       = NULL,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = vc_priv_debug_callback,
        };

        VK_CHECKR(vc_vkCreateDebugUtilsMessengerEXT(ctx->vk_instance, &debug_utils_ci, NULL, &ctx->debugging_messenger), "Could not create debug utils messenger");
        vc_info("Debugging enabled");
    }

    // Create handle pool
    vc_handles_manager_create(&ctx->handles_manager);
    vc_handles_manager_set_destroy_function_usr_data(&ctx->handles_manager, ctx);

    return TRUE;
}

void
vc_ctx_destroy(vc_ctx   *ctx)
{
    vc_info("Destroying a vulkan context");
    vkDeviceWaitIdle(ctx->current_device);
    vc_trace("Destroying all objects");
    vc_handles_manager_destroy(&ctx->handles_manager);
    // Device destruction
    if(ctx->current_device != VK_NULL_HANDLE)
    {
        vc_trace("Destroying device and VMA");
        vmaDestroyAllocator(ctx->main_allocator);
        vkDestroyDevice(ctx->current_device, NULL);
    }

    if(ctx->debugging_enabled)
    {
        vc_trace("Debugging was enabled, destroying debugging messenger");
        vc_vkDestroyDebugUtilsMessengerEXT(ctx->vk_instance, ctx->debugging_messenger, NULL);
    }

    vc_trace("Destroying instance");
    vkDestroyInstance(ctx->vk_instance, NULL);
}

// Checks for layers availability
b8
vc_priv_check_layers(char **layers, u32 count)
{
    u32 total_count = 0;
    vkEnumerateInstanceLayerProperties(&total_count, NULL);
    VkLayerProperties *properties = alloca(sizeof(VkLayerProperties) * total_count);
    vkEnumerateInstanceLayerProperties(&total_count, properties);

    for(u32 i = 0; i < count; i++)
    {
        b8 found = FALSE;
        for(u32 j = 0; j < total_count; j++)
        {
            if(strcmp(layers[i], properties[j].layerName) == 0)
            {
                // Layer i found
                found = TRUE;
            }
        }

        if(!found)
        {
            vc_error("Instance layers: layer '%s' not found", layers[i]);
            return FALSE;
        }
    }

    return TRUE;
}

// Checks for instance extensions availability
b8
vc_priv_check_instance_extensions(char **extensions, u32 count)
{
    u32 total_count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &total_count, NULL);
    VkExtensionProperties *properties = alloca(sizeof(VkLayerProperties) * total_count);
    vkEnumerateInstanceExtensionProperties(NULL, &total_count, properties);

    for(u32 i = 0; i < count; i++)
    {
        b8 found = FALSE;
        for(u32 j = 0; j < total_count; j++)
        {
            if(strcmp(extensions[i], properties[j].extensionName) == 0)
            {
                // Layer i found
                found = TRUE;
            }
        }

        if(!found)
        {
            vc_error("Instance extensions: extension '%s' not found", extensions[i]);
            return FALSE;
        }
    }

    return TRUE;
}

#undef VC_CURRENT_SUBSYS_NAME
#define VC_CURRENT_SUBSYS_NAME "vk_valid"
// Debug utils messenger callback
VKAPI_ATTR VkBool32 VKAPI_CALL
vc_priv_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        vc_trace("[\e[31;1mVULKAN\e[0m]: %s", pCallbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        vc_info("[\e[31;1mVULKAN\e[0m]: %s", pCallbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        vc_warn("[\e[31;1mVULKAN\e[0m]: %s", pCallbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        vc_error("[\e[31;1mVULKAN\e[0m]: %s", pCallbackData->pMessage);
        break;

    default:
        vc_trace("[\e[31;1mVULKAN\e[0m]: %s", pCallbackData->pMessage);
        break;
    }
    return VK_FALSE;
}

