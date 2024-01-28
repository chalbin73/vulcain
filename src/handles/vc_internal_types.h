/*
 * This contains the definition of the structs that represents the object pointed-to by the handle system
 * they should normaly only be used internally.
 */

/*
 * Here names should start with _ and end with _intern to specifiy that they are managed object, by the handle system
 *
 */

#include <vulkan/vulkan.h>

typedef struct
{
    VkSwapchainKHR    swapchain;
} _vc_swapchain_intern;

typedef struct
{
    VkQueue         queue;
    VkQueueFlags    queue_flags;
} _vc_queue_intern;

