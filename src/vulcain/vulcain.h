// clang-format off
#include "../base/base.h"
#include "../base/data_structures/darray.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

// clang-format on

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
    b8                       request_main_queue;
    b8                       request_compute_queue;
    b8                       request_transfer_queue;
    VkPhysicalDeviceType     allowed_types;
    VkPhysicalDeviceFeatures requested_features;
} physical_device_query;

typedef struct
{

} swapchain_desc;

typedef enum
{
    VC_QUEUE_MAIN = 0, // Main queue, supporting graphics, present and compute, main work queue
    VC_QUEUE_COMPUTE,  // Async compute, for compute work that can be async to main work
    VC_QUEUE_TRANSFER, // DMA transfer queues usually
    VC_QUEUE_TYPE_COUNT
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
        u32     indices[VC_QUEUE_TYPE_COUNT];
        f32     priorities[VC_QUEUE_TYPE_COUNT];
        VkQueue queues[VC_QUEUE_TYPE_COUNT];
    } queues;

    struct swapchain_conf
    {
        VkSurfaceFormatKHR       swapchain_format;
        VkPresentModeKHR         present_mode;
        VkExtent3D               swapchain_extent;
        VkFormat                 depth_format;
        u32                      image_count;
        VkSurfaceCapabilitiesKHR capabilities;
    } swapchain_conf;

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

b8  _vc_priv_is_physical_device_suitable(vc_ctx *ctx, physical_device_query query, VkPhysicalDevice phys_device, VkSurfaceKHR surface);
i32 _vc_priv_search_physical_device_queue(vc_ctx *ctx, vc_queue_type type, VkPhysicalDevice phys_device, VkSurfaceKHR surface);