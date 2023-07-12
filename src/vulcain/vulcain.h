#pragma once

#include "../base/base.h"
#include "../base/data_structures/darray.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include "windowing_systems/vc_windowing_systems.h"
#include "vc_handles.h"
#include "../base/data_structures/hashmap.h"
#include <vk_mem_alloc.h>

typedef struct
{
    char                        *engine_name;
    char                        *app_name;
    u32                          app_version;
    u32                          engine_version;
    b8                           enable_debugging;
    u32                          extension_count;
    char                       **extensions;
    b8                           enable_windowing;
    vc_windowing_system_funcs    windowing_system;
} instance_desc;

typedef struct
{
    b8                          request_main_queue;
    b8                          request_compute_queue;
    b8                          request_transfer_queue;
    VkPhysicalDeviceType        allowed_types;
    VkPhysicalDeviceFeatures    requested_features;
} physical_device_query;

typedef enum
{
    VC_QUEUE_MAIN = 0,     // Main queue, supporting graphics, present and compute, main work queue
    VC_QUEUE_COMPUTE,     // Async compute, for compute work that can be async to main work
    VC_QUEUE_TRANSFER,     // DMA transfer queues usually
    VC_QUEUE_IGNORED,     // For barrier
    VC_QUEUE_TYPE_COUNT
} vc_queue_type;

typedef enum
{
    VC_QUEUE_MAIN_BIT     = 0b1,
    VC_QUEUE_COMPUTE_BIT  = 0b10,
    VC_QUEUE_TRANSFER_BIT = 0b100,
} vc_queue_flags;

typedef struct
{
    u32                   binding;
    VkDescriptorType      type;
    VkShaderStageFlags    stage;
} pipe_binding_desc;

typedef enum
{
    VC_PIPELINE_TYPE_COMPUTE = 1,
    VC_PIPELINE_TYPE_GRAPHICS,
} pipeline_type;

typedef struct
{
    u8                         *shader_code;
    u32                         shader_code_length;
    vc_descriptor_set_layout    set_layout;
} compute_pipe_desc;

typedef struct
{
    vc_compute_pipe    pipe;
    u32                groups_x;
    u32                groups_y;
    u32                groups_z;
} compute_dispatch_desc;

typedef enum
{
    VC_MEMORY_HOST_VISIBLE = 0,
    VC_MEMORY_DEVICE_LOCAL_HOST_VISIBLE,
    VC_MEMORY_DEVICE_LOCAL_NOT_VISIBLE,
} memory_visibility;

typedef struct
{
    // TODO: Make this better such that we can select device local, not host visible memeory (for more optimization)
    b8                       require_host_visible;
    b8                       require_device_local;
    VkBufferUsageFlagBits    buffer_usage;
    u64                      size;
} buffer_alloc_desc;

#define VC_IMAGE_CREATE_AUTO_MIP 0xFFFFFFFF

typedef struct
{
    u32                      image_dimension;  // 1D 2D 3D
    VkFormat                 image_format;
    u32                      width;
    u32                      height;
    u32                      depth;
    u32                      mip_levels;
    u32                      layers;
    VkSampleCountFlagBits    sample_count;
    VkImageUsageFlags        image_usage;
    b8                       share;  // To share the resource accross queues
    vc_queue_flags           queues;  // Only used is share is true
    VkImageLayout            layout;  // Layout transition is performed here if not undefined
} image_create_desc;

/* ---------------- Descriptors ---------------- */

// For now those are the same as VkDescriptorBufferInfo, VkDescriptorImageInfo
typedef struct
{
    vc_buffer    buffer;
    b8           whole_buffer;
    // Useless if whole_buffer is true
    u64    offset;
    u64    range;
} descriptor_binding_buffer;

typedef struct
{
    VkSampler        sampler;
    VkImageView      imageView;
    VkImageLayout    imageLayout;
} descriptor_binding_image;

typedef struct
{
    VkDescriptorType      descriptor_type;
    uint32_t              descriptor_count;
    VkShaderStageFlags    stage_flags;

    descriptor_binding_buffer   *buffer_info;
    descriptor_binding_image    *image_info;
} descriptor_binding_desc;

typedef struct
{
    u32                        binding_count;
    descriptor_binding_desc   *bindings;
} descriptor_set_desc;

typedef struct
{
    VkInstance                   vk_instance;
    VkDebugUtilsMessengerEXT     vk_debug_messenger;
    VkSurfaceKHR                 vk_window_surface;
    VkPhysicalDevice             vk_selected_physical_device;
    VkDevice                     vk_device;
    b8                           use_windowing;
    vc_windowing_system_funcs    windowing_system;
    vc_handle_mgr                handle_manager;
    hashmap                      desc_set_layouts_hashmap;
    VkDescriptorPool             vk_main_descriptor_pool;  // TODO: Make a swapping pool system.
    VmaAllocator                 vma_allocator;

    struct queues
    {
        u32              indices[VC_QUEUE_TYPE_COUNT];
        f32              priorities[VC_QUEUE_TYPE_COUNT];
        VkQueue          queues[VC_QUEUE_TYPE_COUNT];
        VkCommandPool    pools[VC_QUEUE_TYPE_COUNT];
    }    queues;

    struct swapchain_conf
    {
        VkSurfaceFormatKHR          swapchain_format;
        VkPresentModeKHR            present_mode;
        VkExtent2D                  swapchain_extent;
        VkFormat                    depth_format;
        u32                         image_count;
        VkSurfaceCapabilitiesKHR    capabilities;
    }    swapchain_conf;

    struct swapchain
    {
        VkSwapchainKHR    vk_swapchain;
        VkImage          *swapchain_images;
        vc_image         *swapchain_image_hndls;
        VkImageView      *swapchain_image_views;
        u32               swapchain_image_count;
    }    swapchain;
} vc_ctx;

/* ---------------- Enum helpers ---------------- */
const char   *vc_priv_VkResult_to_str(VkResult input_value);

#define VK_CHECK(s, m)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        VkResult _res = s;                                                                                             \
        if (_res != VK_SUCCESS)                                                                                        \
        {                                                                                                              \
            ERROR( "VKERROR: '%s' %s:%d error=%d:(%s).", m, __FILE__, __LINE__, _res, vc_priv_VkResult_to_str(_res) ); \
        }                                                                                                              \
    }                                                                                                                  \
    while (0);

#define VK_CHECKR(s, m)                                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        VkResult _res = s;                                                                                             \
        if (_res != VK_SUCCESS)                                                                                        \
        {                                                                                                              \
            ERROR( "VKERROR: '%s' %s:%d error=%d:(%s).", m, __FILE__, __LINE__, _res, vc_priv_VkResult_to_str(_res) ); \
            return FALSE;                                                                                              \
        }                                                                                                              \
    }                                                                                                                  \
    while (0);

#define VK_CHECKH(s, m)                                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        VkResult _res = s;                                                                                             \
        if (_res != VK_SUCCESS)                                                                                        \
        {                                                                                                              \
            ERROR( "VKERROR: '%s' %s:%d error=%d:(%s).", m, __FILE__, __LINE__, _res, vc_priv_VkResult_to_str(_res) ); \
            return VC_NULL_HANDLE;                                                                                     \
        }                                                                                                              \
    }                                                                                                                  \
    while (0);

/* ---------------- ContextS ---------------- */

b8      vc_create_ctx(vc_ctx *ctx, instance_desc *desc, physical_device_query *phys_device_query);
void    vc_destroy_ctx(vc_ctx *ctx);

/* ---------------- Handles ---------------- */

// Destroys any kind of handle based on its destroy function
void    vc_handle_destroy(vc_ctx *ctx, vc_handle hndl);

/* ---------------- Queues ---------------- */

void    vc_queue_wait_idle(vc_ctx *ctx, vc_queue_type type);

/* ---------------- Pipelines ---------------- */

vc_compute_pipe    vc_compute_pipe_create(vc_ctx *ctx, compute_pipe_desc *desc);

/* ---------------- Commands ---------------- */

vc_command_buffer    vc_command_buffer_main_create(vc_ctx *ctx, vc_queue_type queue);
void                 vc_command_buffer_submit(vc_ctx *ctx, vc_command_buffer command_buffer, vc_semaphore wait_on_semaphore, VkPipelineStageFlags *wait_on_stages);
void                 vc_command_buffer_begin(vc_ctx *ctx, vc_command_buffer command_buffer);
void                 vc_command_buffer_end(vc_ctx *ctx, vc_command_buffer command_buffer);
void                 vc_command_buffer_reset(vc_ctx *ctx, vc_command_buffer command_buffer);
void                 vc_command_buffer_compute_pipeline(vc_ctx *ctx, vc_command_buffer command_buffer, compute_dispatch_desc *desc);
void                 vc_command_buffer_bind_descriptor_set(vc_ctx *ctx, vc_command_buffer command_buffer, vc_handle pipeline, vc_descriptor_set desc_set);

/* ---------------- Synchronisation ---------------- */

vc_semaphore    vc_semaphore_create(vc_ctx *ctx);

/* ---------------- Swapchain ---------------- */

void        vc_swapchain_acquire_image(vc_ctx *ctx, u32 *image_id, vc_semaphore acquired_semaphore);
void        vc_swapchain_present_image(vc_ctx *ctx, u32 image_id);
vc_image   *vc_swapchain_get_image_hndls(vc_ctx *ctx);

/* ---------------- Descriptors ---------------- */

vc_descriptor_set_layout    vc_descriptor_set_layout_create(vc_ctx *ctx, descriptor_set_desc desc_set_desc);
vc_descriptor_set           vc_descriptor_set_create(vc_ctx *ctx, descriptor_set_desc desc_set_desc, vc_descriptor_set_layout set_layout);

/* ---------------- Pipelines ---------------- */

void    vc_command_image_pipe_barrier(vc_ctx              *ctx,
                                      vc_command_buffer    command_buffer,
                                      vc_image             image,

                                      VkImageLayout        src_layout,
                                      VkImageLayout        dst_layout,

                                      VkPipelineStageFlags from,
                                      VkPipelineStageFlags to,

                                      VkAccessFlags        src_access,
                                      VkAccessFlags        dst_access,

                                      vc_queue_type        src_queue,
                                      vc_queue_type        dst_queue
                                      );

vc_descriptor_set_layout    vc_priv_desc_set_layout_get(vc_ctx *ctx, VkDescriptorSetLayoutCreateInfo *ci);

/* ---------------- Buffers ---------------- */
vc_buffer    vc_buffer_allocate(vc_ctx *ctx, buffer_alloc_desc alloc_desc);

/* ---------------- Images ---------------- */
vc_image    vc_image_allocate(vc_ctx *ctx, image_create_desc desc);

/* ---------------- Utils ---------------- */