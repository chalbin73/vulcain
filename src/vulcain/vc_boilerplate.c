#include "vulcain.h"

// All "boilerplate" objects : instance, device, queues, so on

b8 vc_create_ctx(vc_ctx *ctx, instance_desc *desc)
{
    //Create instance
    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.pApplicationName = desc->app_name;
    app_info.pEngineName = desc->engine_name;
    app_info.applicationVersion = desc->app_version;
    app_info.apiVersion = VK_API_VERSION_1_2;
    app_info.engineVersion = desc->engine_version;

    VkInstanceCreateInfo inst_ci = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    inst_ci.pApplicationInfo = &app_info;
    inst_ci.enabledExtensionCount = desc->extension_count;
    inst_ci.enabledLayerCount = desc->enable_debugging ? 1 : 0;
    inst_ci.ppEnabledExtensionNames = (const char * const*)desc->extensions;

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
                if(strcmp(props[j].extensionName, desc->extensions[i]) == 0)
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
    return TRUE;
}