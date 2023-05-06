#include "../base/base.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

typedef struct
{
    char  *engine_name;
    char  *app_name;
    u32    app_version;
    u32    engine_version;
    b8     enable_debugging;
    u32    extension_count;
    char **extensions;
} instance_desc;

typedef struct
{
    b8                       supports_graphics;
    b8                       supports_compute;
    b8                       supports_present;
    VkPhysicalDeviceType     allowed_types;
    VkPhysicalDeviceFeatures requested_features;
} physical_device_query;

typedef enum
{
    VC_GRAPHICS_QUEUE = 1,
    VC_COMPUTE_QUEUE,
    VC_PRESENT_QUEUE
} vc_queue_type;

typedef struct
{
    VkInstance               vk_instance;
    VkDebugUtilsMessengerEXT vk_debug_messenger;
    VkSurfaceKHR             vk_window_surface;
    VkPhysicalDevice         vk_selected_physical_device;
    VkDevice                 vk_device;

    struct queues
    {
        u32     graphics_index;
        u32     compute_index;
        u32     present_index;
        VkQueue graphics_queue;
        VkQueue compute_queue;
        VkQueue present_queue;
    } queues;

} vc_ctx;

#define VK_CHECK(s, m)                                                           \
    do                                                                           \
    {                                                                            \
        VkResult _res = s;                                                       \
        if (_res != VK_SUCCESS)                                                  \
        {                                                                        \
            ERROR("VKERROR: '%s' %s:%d error=%d.", m, __FILE__, __LINE__, _res); \
        }                                                                        \
    }                                                                            \
    while (0);
#define VK_CHECKR(s, m)                                                          \
    do                                                                           \
    {                                                                            \
        VkResult _res = s;                                                       \
        if (_res != VK_SUCCESS)                                                  \
        {                                                                        \
            ERROR("VKERROR: '%s' %s:%d error=%d.", m, __FILE__, __LINE__, _res); \
            return FALSE;                                                        \
        }                                                                        \
    }                                                                            \
    while (0);

b8   vc_create_ctx(vc_ctx *ctx, instance_desc *desc);
b8   vc_get_surface_glfw(vc_ctx *ctx, GLFWwindow *window);
b8   vc_select_create_device(vc_ctx *ctx, physical_device_query query);
void vc_destroy_ctx(vc_ctx *ctx);

b8 _vc_priv_is_physical_device_suitable(vc_ctx *ctx, physical_device_query query, VkPhysicalDevice phys_device, VkSurfaceKHR surface);