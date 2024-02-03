/*
 * This contains the definition of the structs that represents the object pointed-to by the handle system
 * they should normaly only be used internally.
 */

/*
 * Here names should start with _ and end with _intern to specifiy that they are managed object, by the handle system
 *
 */

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "../base/base.h"
#include "../vc_windowing.h"
#include "../vulcain.h"

typedef struct
{
    VkSwapchainKHR                            swapchain;

    VkSurfaceKHR                              surface;
    VkSurfaceFormatKHR                        surface_format;
    VkImageUsageFlags                         image_usage;

    vc_windowing_system                       windowing_system;

    u32                                       image_count;
    VkExtent2D                                image_extent;
    vc_image                                 *swapchain_images;

    void                                     *clbk_udata;
    vc_swapchain_creation_callback_func       creation_callback;
    vc_swapchain_destruction_callback_func    destruction_callback;

} _vc_swapchain_intern;

typedef struct
{
    VkQueue         queue;
    VkQueueFlags    queue_flags;
    u32             queue_family_index;
} _vc_queue_intern;

typedef struct
{
    VkCommandPool    pool;
    u32              family_index;
} _vc_command_pool_intern;

typedef struct
{
    VkCommandBuffer    buffer;
    vc_ctx            *record_ctx;
} _vc_command_buffer_intern;

typedef struct
{
    VkSemaphore    semaphore;
    b8             is_timeline;
} _vc_semaphore_intern;

typedef struct
{
    b8               externally_managed; // If the image is managed by an external system like swapchains

    VkImage          image;
    VmaAllocation    alloc;
} _vc_image_intern;

typedef struct
{
    VkPipeline          pipeline;
    VkPipelineLayout    layout;
} _vc_compute_pipeline_intern;

typedef struct
{
    VkDescriptorSet          set;
    VkDescriptorSetLayout    layout;
} _vc_descriptor_set_intern;

