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

/**
 * @brief Main header for vulcain, a "simple", vulkan, wrapper layer
 *        with handle system.
 *
 */

/**
 * @brief Necessary parameters to create a vulkan instance
 *
 */
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

/**
 * @brief Characteristics requested for the physical device slection
 *
 */
typedef struct
{
    b8                          request_main_queue;
    b8                          request_compute_queue;
    b8                          request_transfer_queue;
    VkPhysicalDeviceType        allowed_types;
    VkPhysicalDeviceFeatures    requested_features;
} physical_device_query;

/**
 * @brief A type of vulkan queue (based on what family they are created on)
 *
 */
typedef enum
{
    VC_QUEUE_MAIN = 0,     // Main queue, supporting graphics, present and compute, main work queue
    VC_QUEUE_COMPUTE,     // Async compute, for compute work that can be async to main work
    VC_QUEUE_TRANSFER,     // DMA transfer queues usually
    VC_QUEUE_IGNORED,     // For barrier
    VC_QUEUE_TYPE_COUNT
} vc_queue_type;

/**
 * @brief Queue type Flags
 *
 */
typedef enum
{
    VC_QUEUE_MAIN_BIT     = 0b1,
    VC_QUEUE_COMPUTE_BIT  = 0b10,
    VC_QUEUE_TRANSFER_BIT = 0b100,
} vc_queue_flags;

/**
 * @brief The type of vulkan pipeline
 *
 */
typedef enum
{
    VC_PIPELINE_TYPE_COMPUTE = 1,
    VC_PIPELINE_TYPE_GRAPHICS,
} vc_pipeline_type;

/* ---------------- Graphics/Render pass/Frambuffer ---------------- */

/**
 * @brief Represents a set of attachments, that will be used during a rendering process
 *
 */
typedef struct
{
    u32         attachment_count;
    vc_image   *attachments;
} render_attachments_set;


/**
 * @brief Used to reference a renderpass attatchment from a subpass
 * @deprecated VkAttachmentReference is used now, as it fullfills this part
 */
typedef struct
{
    /** @brief The index of the attachement in the attachment set */
    u32              id;
    /** @brief The layout in which the attachment shall be transitioned to for this subpass */
    VkImageLayout    layout;
} subpass_attachment_reference;

/**
 * @brief Gives the necessary parameters for an attachment when creating a render pass
 *
 */
typedef struct
{
    VkAttachmentLoadOp     load_op;
    VkAttachmentStoreOp    store_op;

    VkAttachmentLoadOp     stencil_load_op;
    VkAttachmentStoreOp    stencil_store_op;

    /** @brief The layout in which the image will be upon entering the render pass */
    VkImageLayout          initial_layout;
    /** @brief The layout in which the image shall be upon exiting the render pass */
    VkImageLayout          final_layout;
} render_pass_attachment_params;

/**
 * @brief Describes a single subpass
 *
 */
typedef struct
{
    vc_pipeline_type         pipline_type;

    u32                      input_attachment_count;
    VkAttachmentReference   *input_attachment_refs;

    u32                      color_attachment_count;
    VkAttachmentReference   *color_attachment_refs;

    u32                      preserve_attachment_count;
    u32                     *preserve_attachment_ids;

    VkAttachmentReference   *depth_stencil_attachment_ref;

} subpass_desc;

/**
 * @brief Describes a single subpass dependency
 *
 */
typedef struct
{
    /** @brief The id of the subpass @code{dst_id} depends on, or VK_SUBPASS_EXTERNAL */
    u32                        src_id;

    /** @brief The id of the subpass, or VK_SUBPASS_EXTERNAL which depends on @code{src_id} */
    u32                        dst_id;

    /** @brief The stages that need to be executed before execution is unlocked on destination subpass */
    VkPipelineStageFlagBits    src_stages;

    /** @brief The stages that are allowed to be executed before the source subpass stages */
    VkPipelineStageFlagBits    dst_stages;

    /** @brief Accesses that shall made be available */
    VkAccessFlags              src_access;

    /** @brief Access to which memory writes (including src_accesses) must be made available to */
    VkAccessFlags              dst_access;
} subpass_dependency_desc;

/**
 * @brief Describes the creation of a vulkan render pass
 *
 */
typedef struct
{
    render_attachments_set           attachment_set;

    /** @brief Parameters for each of the attachment in the attchment set, indexed in the same way */
    render_pass_attachment_params   *attachment_desc;

    u32                              subpass_count;
    /** @brief Defines a subpass */
    subpass_desc                    *subpasses_desc;

    u32                              subpass_dependency_count;
    subpass_dependency_desc         *subpass_dependencies;

} render_pass_desc;

typedef struct
{
    vc_render_pass            render_pass;
    render_attachments_set    attachment_set;
    u32                       layers; // Not sure about this one
} framebuffer_desc;

/**
 * @brief Describes the creation of a vulkan compute pipeline
 *
 */
typedef struct
{
    u8                         *shader_code;
    u32                         shader_code_length;
    vc_descriptor_set_layout    set_layout;
} compute_pipe_desc;

/**
 * @brief Description of a compute pipeline dispatch
 *
 */
typedef struct
{
    vc_compute_pipe    pipe;
    u32                groups_x;
    u32                groups_y;
    u32                groups_z;
} compute_dispatch_desc;

// Wtf is that ... <DEPRECATED>, might be used latter
typedef enum
{
    VC_MEMORY_HOST_VISIBLE = 0,
    VC_MEMORY_DEVICE_LOCAL_HOST_VISIBLE,
    VC_MEMORY_DEVICE_LOCAL_NOT_VISIBLE,
} memory_visibility;

/**
 * @brief Parameters for vulkan buffer allocation
 *
 */
typedef struct
{
    // TODO: Make this better such that we can select device local, not host visible memeory (for more optimization)
    b8                       require_host_visible;
    b8                       require_device_local;
    VkBufferUsageFlagBits    buffer_usage;
    u64                      size;
    b8                       share;
    vc_queue_flags           queues;
} buffer_alloc_desc;

/**
 * @brief If given as mip_levels when creating image, the number of mip levels is selected automaticaly
 *
 */
#define VC_IMAGE_CREATE_AUTO_MIP 0xFFFFFFFF

/**
 * @brief Parameters for image creatin
 *
 */
typedef struct
{
    u32                      image_dimension;  // 1D 2D 3D
    VkFormat                 image_format;
    u32                      width;
    u32                      height;
    u32                      depth;
    u32                      mip_levels;
    u32                      layer_count;
    VkSampleCountFlagBits    sample_count;
    VkImageUsageFlags        image_usage;
    b8                       share;  // To share the resource accross queues
    vc_queue_flags           queues;  // Only used is share is true
    VkImageLayout            layout;  // Layout transition is performed here if not undefined
} image_create_desc;

/**
 * @brief Parameters for swapchain creation
 *
 */
typedef struct
{
    VkImageUsageFlags    image_usage;
    VkImageLayout        initial_images_layout;

} swapchain_creation_desc;

/* ---------------- Descriptors ---------------- */

/**
 * @brief Info for a buffer binding
 *
 */
typedef struct
{
    vc_buffer    buffer;
    b8           whole_buffer;
    // Useless if whole_buffer is true
    u64          offset;
    u64          range;
} descriptor_binding_buffer;

/**
 * @brief Info for an image binding
 *
 */
typedef struct
{
    vc_image         image;
    VkImageLayout    layout;
} descriptor_binding_image;

/**
 * @brief Info for a shader binding
 * @note @c buffer_info and @c{image_info} can be null when creating a descriptor set layout
 */
typedef struct
{
    VkDescriptorType             descriptor_type;
    uint32_t                     descriptor_count;
    VkShaderStageFlags           stage_flags;

    descriptor_binding_buffer   *buffer_info;
    descriptor_binding_image    *image_info;
} descriptor_binding_desc;

/**
 * @brief Describes a descriptor set/set layout
 *
 */
typedef struct
{
    u32                        binding_count;
    descriptor_binding_desc   *bindings;
} descriptor_set_desc;

/**
 * @brief Id of a swapchain image (used to know where the id can be used coherently)
 *
 */
typedef u32 vc_swp_img_id;

/**
 * @brief The context for all vulcain operations
 *
 */
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

    // Data relative to the queues
    struct queues
    {
        u32              indices[VC_QUEUE_TYPE_COUNT];
        f32              priorities[VC_QUEUE_TYPE_COUNT];
        VkQueue          queues[VC_QUEUE_TYPE_COUNT];
        VkCommandPool    pools[VC_QUEUE_TYPE_COUNT];
    }    queues;

    // Data relative to the swapchain parameters
    struct swapchain_conf
    {
        VkSurfaceFormatKHR          swapchain_format;
        VkPresentModeKHR            present_mode;
        VkExtent2D                  swapchain_extent;
        VkFormat                    depth_format;
        u32                         image_count;
        VkSurfaceCapabilitiesKHR    capabilities;
    }    swapchain_conf;

    // Contains objects tied to the swapchain
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
const char            *vc_priv_VkColorSpaceKHR_to_str(VkColorSpaceKHR    input_value);
const char            *vc_priv_VkFormat_to_str(VkFormat    input_value);
const char            *vc_priv_VkResult_to_str(VkResult    input_value);
VkPipelineBindPoint    vc_priv_pipeline_type_to_bind_point(vc_pipeline_type    type);

/* ---------------- Error checking ---------------- */
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

/* ---------------- Context ---------------- */

/**
 * @brief Creates a context for vulcain
 *
 * @param ctx The ouput context
 * @param desc The description of the instance creation
 * @param phys_device_query The requested characteristics for the physical device
 * @return b8 Success
 */
b8                          vc_create_ctx(vc_ctx *ctx, instance_desc *desc, physical_device_query *phys_device_query);

/**
 * @brief Destroys the vulcain context and all subsequent vulkan objects
 *
 * @param ctx A pointer to the vulcain context
 */
void                        vc_destroy_ctx(vc_ctx   *ctx);

/* ---------------- Handles ---------------- */

/**
 * @brief Destroys a vulcain handle and its object
 *
 * @param ctx
 * @param hndl Any handle
 */
void                        vc_handle_destroy(vc_ctx *ctx, vc_handle hndl);

/* ---------------- Queues ---------------- */

/**
 * @brief Waits for a queue to be idle
 *
 * @param ctx
 * @param type The queue family to wait on
 */
void                        vc_queue_wait_idle(vc_ctx *ctx, vc_queue_type type);

/* ---------------- Pipelines ---------------- */

/**
 * @brief Creates a vulkan compute pipeline
 *
 * @param ctx
 * @param desc The description of the pipeline
 * @return vc_compute_pipe The pipeline handle
 */
vc_compute_pipe             vc_compute_pipe_create(vc_ctx *ctx, compute_pipe_desc *desc);

/* ---------------- Commands ---------------- */

/**
 * @brief Creates a command buffer
 *
 * @param ctx
 * @param queue The queue on which to create the command buffer
 * @return vc_command_buffer A handle to the command buffer
 */
vc_command_buffer           vc_command_buffer_main_create(vc_ctx *ctx, vc_queue_type queue);

/**
 * @brief Submits a command buffer
 *
 * @param ctx
 * @param command_buffer The command buffer to submit
 * @param wait_on_semaphore The semaphore to wait on before submitting
 * @param wait_on_stages The stages on which to wait upon semaphore signaling
 */
void                        vc_command_buffer_submit(vc_ctx *ctx, vc_command_buffer command_buffer, vc_semaphore wait_on_semaphore, VkPipelineStageFlags *wait_on_stages);

/**
 * @brief Begins a command buffer
 *
 * @param ctx
 * @param command_buffer The command buffer to begin
 */
void                        vc_command_buffer_begin(vc_ctx *ctx, vc_command_buffer command_buffer);

/**
 * @brief Ends a command buffer
 *
 * @param ctx
 * @param command_buffer The command buffer to end
 */
void                        vc_command_buffer_end(vc_ctx *ctx, vc_command_buffer command_buffer);

/**
 * @brief Resets a command buffer
 *
 * @param ctx
 * @param command_buffer The command buffer to reset
 */
void                        vc_command_buffer_reset(vc_ctx *ctx, vc_command_buffer command_buffer);

/**
 * @brief Commands a compute pipeline dispatch
 *
 * @param ctx
 * @param command_buffer The command buffer
 * @param desc The parameters of the dispatch
 */
void                        vc_command_buffer_compute_pipeline(vc_ctx *ctx, vc_command_buffer command_buffer, compute_dispatch_desc *desc);

/**
 * @brief Commands the binding a descriptor set
 *
 * @param ctx
 * @param command_buffer The command buffer
 * @param pipeline The pipline which will use the command buffer (Only uses the layout of this pipline)
 * @param desc_set The descriptor set to bind
 */
void                        vc_command_buffer_bind_descriptor_set(vc_ctx *ctx, vc_command_buffer command_buffer, vc_handle pipeline, vc_descriptor_set desc_set);

/**
 * @brief Copies one image to another
 *
 * @param ctx
 * @param command_buffer
 * @param src The source image
 * @param dst The destination inmage
 * @note This copy is automatic, and different images sizes may result in validation layers complaining
 */
void                        vc_command_simple_image_copy(vc_ctx *ctx, vc_command_buffer command_buffer, vc_image src, vc_image dst);

/* ---------------- Synchronisation ---------------- */

/**
 * @brief Creates a semaphore
 *
 * @param ctx
 * @return vc_semaphore The semaphore
 */
vc_semaphore                vc_semaphore_create(vc_ctx   *ctx);

/* ---------------- Swapchain ---------------- */

/**
 * @brief Sets up the image views for every swapchain image
 *
 * @param ctx
 * @note This is currently already done during swapchain creation
 */
void                        vc_swapchain_create_full_image_views(vc_ctx   *ctx);

/**
 * @brief Acquires an image on the swapchains
 *
 * @param ctx
 * @param[out] image_id The id (index) of the image that has will be acquired
 * @param acquired_semaphore The semaphore to signal upon acquire of the image
 */
void                        vc_swapchain_acquire_image(vc_ctx *ctx, vc_swp_img_id *image_id, vc_semaphore acquired_semaphore);

/**
 * @brief Presents an image on the screen
 *
 * @param ctx
 * @param image_id The id of the previously acquired image
 */
void                        vc_swapchain_present_image(vc_ctx *ctx, vc_swp_img_id image_id);

/**
 * @brief Gets a pointer to an array of image handles to the swapchain images (can be indexed using @c{vc_swp_img_id})
 *
 * @param ctx
 * @return vc_image* A pointer to an array of images
 */
vc_image                   *vc_swapchain_get_image_hndls(vc_ctx   *ctx);

/**
 * @brief The number of images in teh swapchain
 *
 * @param ctx
 * @return u32 The number of images in the swapchain
 */
u32                         vc_swapchain_image_count(vc_ctx   *ctx);

/* ---------------- Descriptors ---------------- */

/**
 * @brief Creates a descriptor set layout (to be used during pipeline creation)
 *
 * @param ctx
 * @param desc_set_desc Description of the set layout
 * @return vc_descriptor_set_layout The set layout
 * @note @c{buffer_info} and @c{image_info} can be NULL during set layout creation
 */
vc_descriptor_set_layout    vc_descriptor_set_layout_create(vc_ctx *ctx, descriptor_set_desc desc_set_desc);

/**
 * @brief Creates a descriptor set
 *
 * @param ctx
 * @param desc_set_desc Description of the set
 * @param set_layout The set layout for this set
 * @return vc_descriptor_set The descriptor set
 */
vc_descriptor_set           vc_descriptor_set_create(vc_ctx *ctx, descriptor_set_desc desc_set_desc, vc_descriptor_set_layout set_layout);

/* ---------------- Pipelines ---------------- */

/**
 * @brief Places an image barrier in a command buffer
 *
 * @param ctx
 * @param command_buffer The command buffer
 * @param image The image on which to place the barrier
 * @param src_layout The source layout of the image
 * @param dst_layout The destination layout of the image
 * @param from On what pipeline stages will this barrier depend on
 * @param to What pipeline stages will depend on this barrier
 * @param src_access The accesses made on the image by the previous pipline stages
 * @param dst_access The accesses made on the image by the next pipeline stages
 * @param src_queue The source queue (can be @c{VC_QUEUE_IGNORED})
 * @param dst_queue The destination queue (can be @c{VC_QUEUE_IGNORED})
 * @note If @c{src_queue} and @c{dst_queue} are @c{VC_QUEUE_IGNORED} no queue ownership transfer is made
 */
void                        vc_command_image_pipe_barrier(vc_ctx                 *ctx,
                                                          vc_command_buffer       command_buffer,
                                                          vc_image                image,

                                                          VkImageLayout           src_layout,
                                                          VkImageLayout           dst_layout,

                                                          VkPipelineStageFlags    from,
                                                          VkPipelineStageFlags    to,

                                                          VkAccessFlags           src_access,
                                                          VkAccessFlags           dst_access,

                                                          vc_queue_type           src_queue,
                                                          vc_queue_type           dst_queue
                                                          );

//TODO: Get rid of this function here
/**
 * @brief <PRIVATE FUNC> Creates, or reuses a descriptor set layout based on the info
 *
 * @param ctx
 * @param ci The create info
 * @return vc_descriptor_set_layout Handle to set layout
 */
vc_descriptor_set_layout    vc_priv_desc_set_layout_get(vc_ctx *ctx, VkDescriptorSetLayoutCreateInfo *ci);

/* ---------------- Buffers ---------------- */

/**
 * @brief Allocates a buffer
 *
 * @param ctx
 * @param alloc_desc The allocation parameters
 * @return vc_buffer A handle to the buffer
 */
vc_buffer                   vc_buffer_allocate(vc_ctx *ctx, buffer_alloc_desc alloc_desc);

/* ---------------- Images ---------------- */

/**
 * @brief Allocates an image
 *
 * @param ctx
 * @param desc Descripion of the image
 * @return vc_image A handle to the imaage
 */
vc_image                    vc_image_allocate(vc_ctx *ctx, image_create_desc desc);

/**
 * @brief Creates the full image view for an image, that is, the image view viewing the full subRessources of the image (the one most commonly used)
 *
 * @param ctx
 * @param img The image on which to setup the full image view
 * @note The image object handle by @c{vc_image} contains a default image view (setup by this function)
 */
void                        vc_image_create_full_image_view(vc_ctx *ctx, vc_image img);

/* ---------------- Graphics ---------------- */
/* ---------------- Render pass ---------------- */
vc_render_pass              vc_render_pass_create(vc_ctx *ctx, render_pass_desc desc);

/* ---------------- Utils ---------------- */


/**
 * @brief Converts a set of vc_queue_flags to a list of queue indices according to the set bits
 *
 * @param ctx
 * @param flags The flags
 * @param[out] ids A pointer to a allocated emptly list of u32s
 */
void                        vc_queue_flags_to_queue_indices_list(vc_ctx *ctx, vc_queue_flags flags, u32 *ids);

/**
 * @brief Returns the number of set bits in a flag
 *
 * @param flag The flag
 * @returns u32 The number of set bits in flag
 */
u32                         vc_u32_flags_set_bits(u32    flag);