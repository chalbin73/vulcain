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
 *
 * NOTE: A architecture note :
 * I decided that I will not use the "private struct" pattern :
 * Even though this lets user access the members of the struct.
 * If users want to fuck everything up by accesssing members, then so be it
 * If they want to keep everything working, then they just shouldn't access the members
 * But I want to be able to access everything in the struct IF THEY WANT.
 * Also, this gives more control on where to store thoses structs (stack, heap, custom allocator)
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
 * @brief Characterisitcs of a newly created swapchain used in the callback
 *
 */
typedef struct
{
    u32    width;
    u32    height;
    u32    image_count;
} swapchain_created_info;

// Forward declaration for callbacks
typedef struct vc_ctx vc_ctx;

/**
 * @brief Callback function used when the swapchain is being recreated
 *
 */
typedef void (*swapchain_recreated_callback_func)(vc_ctx *ctx, void *user_data, swapchain_created_info info);
typedef void (*swapchain_destroyed_callback_func)(vc_ctx *ctx, void *user_data);

/**
 * @brief Query for a swapchain configuration
 *
 */
typedef struct
{
    // Reserved
} swapchain_configuration_query;

/**
 * @brief Description of a swapchain creation/recreation
 *
 */
typedef struct swapchain_desc
{

    /** @brief Called when swapchain is created/recreated, used to create swapchain dependent objects */
    swapchain_recreated_callback_func    recreation_callback;

    /** @brief Called when swapchain is destroyed, used to free user allocated data (handle created during recreation callback are automatically freed) */
    swapchain_destroyed_callback_func    destruction_callack;
    void                                *callback_user_data;

    VkImageUsageFlags                    swapchain_images_usage;

    // TODO: Add some format query stuff
} swapchain_desc;

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
    u32              attachment_count;
    vc_image_view   *attachments;
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
    VkAttachmentLoadOp       load_op;
    VkAttachmentStoreOp      store_op;

    VkAttachmentLoadOp       stencil_load_op;
    VkAttachmentStoreOp      stencil_store_op;

    /** @brief The layout in which the image will be upon entering the render pass */
    VkImageLayout            initial_layout;
    /** @brief The layout in which the image shall be upon exiting the render pass */
    VkImageLayout            final_layout;

    VkFormat                 attachment_format;
    VkSampleCountFlagBits    attachment_sample_counts;
} render_pass_attachment_params;

/**
 * @brief Describes a single subpass
 *
 */
typedef struct
{
    /** @brief The type of pipline used in this subpass */
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
    u32                              attachment_count;

    /** @brief Parameters for each of the attachment */
    render_pass_attachment_params   *attachment_desc;

    u32                              subpass_count;
    /** @brief Defines a subpass */
    subpass_desc                    *subpasses_desc;

    u32                              subpass_dependency_count;
    subpass_dependency_desc         *subpass_dependencies;

} render_pass_desc;

/**
 * @brief Describes a framebuffer
 *
 */
typedef struct
{
    /** @brief The render pass with which the frambuffer is compatible */
    vc_render_pass            render_pass;

    /** @brief The set of attachments is this frambuffer */
    render_attachments_set    attachment_set;
    u32                       layers; // Not sure about this one
} framebuffer_desc;

typedef struct
{
    /** @brief The format of the attribute */
    VkFormat    format;
    /** @brief The number of byte between the start of the attribute and the start of the binding */
    u32         offset;
} graphics_pipeline_in_attrib;

typedef struct
{
    /** @brief The byte stride between consecutive elements within the buffer */
    u32                            stride;
    u32                            attribute_count;
    graphics_pipeline_in_attrib   *attributes;

    VkVertexInputRate              input_rate;
} graphics_pipeline_in_binding;

/**
 * @brief Describes the shaders used in a graphical pipeline
 *
 */
typedef struct
{
    u8           *vertex_code;
    u64           vertex_code_size;
    const char   *vertex_entry_point; // Null terminated

    u8           *fragment_code;
    u64           fragment_code_size;
    const char   *fragment_entry_point; // Null terminated
} graphics_pipeline_code_desc;

/**
 * @brief Describes the creation of a graphical pipeline
 * TODO: Document this struct's fields
 */
typedef struct
{

    /** @brief The code of the programmable pipeline stages */
    graphics_pipeline_code_desc            shader_code;

    u32                                    set_layout_count;
    vc_descriptor_set_layout              *set_layouts;

    /** @brief The rendere pass with which this pipeline is used */
    vc_render_pass                         render_pass;

    /** @brief The subpass in which this pipeline will be executed */
    u32                                    subpass_index;

    u32                                    vertex_input_binding_count;
    graphics_pipeline_in_binding          *vertex_input_bindings;

    VkPrimitiveTopology                    topology;

    // Ignored if dynamic
    u32                                    viewport_scissor_count;
    VkViewport                            *viewports;
    VkRect2D                              *scissors;

    b8                                     depth_test;
    b8                                     depth_write;
    VkCompareOp                            depth_compare_op;
    b8                                     depth_bound_test_enable;
    f32                                    depth_bounds_min;
    f32                                    depth_bounds_max;

    b8                                     stencil_test;
    VkStencilOpState                       front_faces_stencil_op;
    VkStencilOpState                       back_faces_stencil_op;

    b8                                     enable_depth_clamp;
    VkPolygonMode                          polygon_mode;
    VkCullModeFlagBits                     cull_mode;
    VkFrontFace                            front_face;
    f32                                    line_width;

    b8                                     enable_depth_bias;
    f32                                    depth_bias_clamp;
    f32                                    depth_bias_constant;
    f32                                    depth_bias_slope;

    VkSampleCountFlagBits                  sample_count;
    b8                                     sample_shading;
    f32                                    sample_shading_min_factor;

    u32                                    attchment_count;
    VkPipelineColorBlendAttachmentState   *attchment_blends;
    f32                                    blend_constants[4];

    u32                                    dynamic_state_count;
    VkDynamicState                        *dynamic_states;

} graphics_pipeline_desc;

typedef struct
{
    vc_render_pass       pass;
    vc_framebuffer       frambuffer;
    VkRect2D             render_area;
    u32                  clear_value_count;
    VkClearValue        *clear_values;
    VkSubpassContents    subpass_contents;
} render_pass_begin_desc;

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


/**
 * @brief Parameters for vulkan buffer allocation
 *
 */
typedef struct
{
    VkMemoryPropertyFlags       required_properties;
    VkBufferUsageFlagBits       buffer_usage;
    u64                         size;
    b8                          share;
    vc_queue_flags              queues;
    VmaAllocationCreateFlags    allocation_flags;
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
 * @brief Sampler creation description
 */
typedef struct
{
    VkFilter                mag_filter;
    VkFilter                min_filter;
    VkSamplerMipmapMode     mipmap_mode;
    VkSamplerAddressMode    address_mode_u;
    VkSamplerAddressMode    address_mode_v;
    VkSamplerAddressMode    address_mode_w;
    float                   mip_lod_bias;
    VkBool32                anisotropy_enable;
    float                   max_anisotropy;
    VkBool32                compare_enable;
    VkCompareOp             compare_op;
    float                   min_lod;
    float                   max_lod;
    VkBorderColor           border_color;
    VkBool32                unnormalized_coordinates;
} sampler_desc;


#define VC_COMPONENT_MAPPING_ID                        \
    (VkComponentMapping){ .a = VK_COMPONENT_SWIZZLE_A, \
                          .r = VK_COMPONENT_SWIZZLE_R, \
                          .g = VK_COMPONENT_SWIZZLE_G, \
                          .b = VK_COMPONENT_SWIZZLE_B }



/**
 * @brief Image view creation info
 */
typedef struct
{
    VkImageViewType            view_type;

    VkImageSubresourceRange    subresource_range;
    VkComponentMapping         component_mapping;
} image_view_desc;

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
    vc_image_sampler    sampler;
    vc_image_view       image_view;
    VkImageLayout       layout;
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
 * @brief Allocator for descriptor sets (functions are private)
 * TODO: Consider making this a handled object.
 *
 */
typedef struct
{
    VkDescriptorPool   *used_pools; // darray
    VkDescriptorPool   *free_pools; // darray

    VkDescriptorPool    current_pool;
} vc_descriptor_set_allocator;

/*
 * @brief Caches descriptor set layouts, reuses them, implemented in vc_descriptor_set_layout_cache.c
 *
 */
typedef struct
{
    /* @brief maps descriptor set layout info to descriptor layouts */
    hashmap                  set_layout_map;
    VkDescriptorSetLayout   *layouts; // darray
} vc_descriptor_set_layout_cache;

/**
 * @brief Id of a swapchain image (used to know where the id can be used coherently)
 *
 */
typedef u32 vc_swp_img_id;

typedef struct
{
    VkSurfaceFormatKHR          swapchain_format;
    VkPresentModeKHR            present_mode;
    VkExtent2D                  swapchain_extent;
    VkFormat                    depth_format;
    u32                         image_count;
    VkSurfaceCapabilitiesKHR    capabilities;
} swapchain_configuration;

// No typedef because of forward decl
/**
 * @brief The context for all vulcain operations
 *
 */
struct vc_ctx
{
    VkInstance                        vk_instance;
    b8                                debug_enabled;
    VkDebugUtilsMessengerEXT          vk_debug_messenger;
    VkSurfaceKHR                      vk_window_surface;
    VkPhysicalDevice                  vk_selected_physical_device;
    VkDevice                          vk_device;
    b8                                use_windowing;
    vc_windowing_system_funcs         windowing_system;
    vc_handle_mgr                     handle_manager;
    VmaAllocator                      vma_allocator;
    vc_descriptor_set_allocator       set_allocator;
    vc_descriptor_set_layout_cache    set_layout_cache;

    // Data relative to the queues
    struct queues
    {
        u32              indices[VC_QUEUE_TYPE_COUNT];
        f32              priorities[VC_QUEUE_TYPE_COUNT];
        VkQueue          queues[VC_QUEUE_TYPE_COUNT];
        VkCommandPool    pools[VC_QUEUE_TYPE_COUNT]; // TODO: Add support to create custom pools
    }
                               queues;

    // Data relative to the swapchain parameters
    swapchain_configuration    swapchain_conf;

    // Contains objects tied to the swapchain
    struct swapchain
    {
        VkSwapchainKHR    vk_swapchain;
        u32               swapchain_image_count;
        swapchain_desc    desc;

        vc_image         *swapchain_image_hndls;
        vc_image_view    *swapchain_image_view_hndls;
    }
    swapchain;
};

/* ---------------- Enum helpers ---------------- */
const char            *vc_priv_VkColorSpaceKHR_to_str(VkColorSpaceKHR    input_value);
const char            *vc_priv_VkFormat_to_str(VkFormat    input_value);
const char            *vc_priv_VkResult_to_str(VkResult    input_value);
VkPipelineBindPoint    vc_priv_pipeline_type_to_bind_point(vc_pipeline_type    type);
char                  *vc_priv_VkDebugUtilsMessageSeverityFlagBitsEXT_to_str(VkDebugUtilsMessageSeverityFlagBitsEXT    input_value);
char                  *vc_priv_VkDebugUtilsMessageSeverityFlagBitsEXT_to_prefix_str(VkDebugUtilsMessageSeverityFlagBitsEXT    input_value);

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
 * @brief Creates a command buffer in one of the main (default) pools based on the specified queues
 *
 * @param ctx
 * @param queue The queue on which to create the command buffer
 * @param level The command buffer level
 * @return vc_command_buffer A handle to the command buffer
 */
vc_command_buffer vc_command_buffer_main_create(vc_ctx *ctx, vc_queue_type queue, VkCommandBufferLevel level);

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

/**
 * @brief Begins a render pass object
 *
 * @param ctx
 * @param command_buffer
 * @param desc
 */
void                        vc_command_render_pass_begin(vc_ctx *ctx, vc_command_buffer command_buffer, render_pass_begin_desc desc);

/**
 * @brief Ends a render pass object (that was previously begun)
 *
 * @param ctx
 * @param command_buffer
 */
void                        vc_command_render_pass_end(vc_ctx *ctx, vc_command_buffer command_buffer);

/**
 * @brief Binds a pipline (any type)
 *
 * @param ctx
 * @param command_buffer
 * @param pipe The pipeline handle, either compute or graphics
 */
void                        vc_command_pipeline_bind(vc_ctx *ctx, vc_command_buffer command_buffer, vc_handle pipe);

/**
 * @brief Dynamically sets a set of viewports (set to dynamic during pipeline creation)
 *
 * @param ctx
 * @param command_buffer
 * @param viewport_count The number of viewports
 * @param viewports The viewports
 */
void                        vc_command_dyn_set_viewport(vc_ctx *ctx, vc_command_buffer command_buffer, u32 viewport_count, VkViewport *viewports);

/**
 * @brief Dynamically sets a set of scissors (set to dynamic during pipeline creation)
 *
 * @param ctx
 * @param command_buffer
 * @param viewport_count The number of scissors
 * @param viewports The scissors
 */
void                        vc_command_dyn_set_scissors(vc_ctx *ctx, vc_command_buffer command_buffer, u32 scissor_count, VkRect2D *scissors);

/**
 * @brief Send a draw call (vkCmdDraw)
 *
 * @param ctx
 * @param command_buffer
 * @param vertex_count The number of vertices to draw
 * @param instance_count The number of instance to draw
 * @param first_vertex The index of the first vertex
 * @param first_instance The index of the first instance
 */
void                        vc_command_draw(vc_ctx *ctx, vc_command_buffer command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);


/**
 * @brief Draws a indexed mesh, vertex buffer and index buffer must have been bound before
 *
 * @param ctx
 * @param command_buffer
 * @param index_count The number of indices to draw
 * @param instance_count
 * @param first_index
 * @param vertex_offset
 * @param first_instance
 */
void                        vc_command_draw_indexed(vc_ctx *ctx, vc_command_buffer command_buffer, u32 index_count, u32 instance_count, u32 first_index, int32_t vertex_offset, u32 first_instance);

/**
 * @brief Copies a buffer into another
 *
 * @param ctx
 * @param cmd_buf
 * @param src The source buffer
 * @param dst The destination buffer
 * @param region_count The number of buffer regions to copy
 * @param regions The regions to copy
 */
void                        vc_command_buffer_copy(vc_ctx *ctx, vc_command_buffer cmd_buf, vc_buffer src, vc_buffer dst, u32 region_count, VkBufferCopy *regions);

void                        vc_command_bind_vertex_buffer(vc_ctx *ctx, vc_command_buffer command_buffer, vc_buffer buffer, u32 binding, u64 offset);
void                        vc_command_bind_index_buffer(vc_ctx *ctx, vc_command_buffer command_buffer, vc_buffer buffer, u64 offset, VkIndexType index_type);

void                        vc_command_copy_buffer_to_image(vc_ctx *ctx, vc_command_buffer command_buffer, vc_buffer src, vc_image dst, VkImageLayout dst_layout, u32 region_count, VkBufferImageCopy *regions);


void                        vc_command_execute_secondary_buffers(vc_ctx *ctx, vc_command_buffer command_buffer, u32 command_buffer_count, vc_command_buffer *secondary_buffers);
void                        vc_command_execute_secondary_buffer(vc_ctx *ctx, vc_command_buffer command_buffer, vc_command_buffer secondary_buffer);

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
 * @brief Selects a swapchain configuration, this function can be called once at the beginning, and all the next creations/recreations will used the queried conf.
 *
 * @param ctx
 * @param desc The swapchain description
 */
void                        vc_swapchain_setup(vc_ctx *ctx, swapchain_configuration_query query);

/**
 * @brief Returns the retrieved configuration after setting up the swapchain
 *
 * @param ctx
 * @return swapchain_configuration
 */
swapchain_configuration     vc_swapchain_configuration_get(vc_ctx   *ctx);

/**
 * @brief Actually creates a swapchain, that can be used later. This function should usally be called once, the destruction/creation is then handled through present/acquire/recreation operations
 *
 * @param ctx
 * @param desc
 */
void                        vc_swapchain_commit(vc_ctx *ctx, swapchain_desc desc);

/**
 * @brief Acquires an image on the swapchains
 *
 * @param ctx
 * @param[out] image_id The id (index) of the image that has will be acquired
 * @param acquired_semaphore The semaphore to signal upon acquire of the image
 * @returns b8 FALSE if a swapchain recreation was made, thus the frame need to be cancelled, and restarted (image_id would be null is this case)
 */
b8                          vc_swapchain_acquire_image(vc_ctx *ctx, vc_swp_img_id *image_id, vc_semaphore acquired_semaphore);

/**
 * @brief Presents an image on the screen
 *
 * @param ctx
 * @param image_id The id of the previously acquired image
 * @returns b8 FALSE if a swapchain recreation was made, thus the frame need to be cancelled, and restarted
 */
b8                          vc_swapchain_present_image(vc_ctx *ctx, vc_swp_img_id image_id);

/**
 * @brief Gets a pointer to an array of image handles to the swapchain images (can be indexed using @c{vc_swp_img_id})
 *
 * @param ctx
 * @return vc_image* A pointer to an array of images
 */
vc_image                   *vc_swapchain_get_image_hndls(vc_ctx   *ctx);

/**
 * @brief Gets a pointer to an array of image view handles to the swapchain image views (can be indexed using @c{vc_swp_img_id})
 *
 * @param ctx
 * @return vc_image* A pointer to an array of image views
 */
vc_image_view              *vc_swapchain_get_image_view_hndls(vc_ctx   *ctx);

/**
 * @brief The number of images in teh swapchain
 *
 * @param ctx
 * @return u32 The number of images in the swapchain
 */
u32                         vc_swapchain_image_count(vc_ctx   *ctx);

/**
 * @brief Forces a complete destruction/creation cycle of the pipline, thus calling the callback. If this is called during a frame, then the frame most likely needs to be cancelled
 *
 * @param ctx
 */
void                        vc_swapchain_force_recreation(vc_ctx   *ctx);

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
 * @param subresource_range The subresource range on which to apply the memory dependency
 * @note If @c{src_queue} and @c{dst_queue} are @c{VC_QUEUE_IGNORED} no queue ownership transfer is made
 */
void                        vc_command_image_pipe_barrier(vc_ctx                    *ctx,
                                                          vc_command_buffer          command_buffer,
                                                          vc_image                   image,

                                                          VkImageLayout              src_layout,
                                                          VkImageLayout              dst_layout,

                                                          VkPipelineStageFlags       from,
                                                          VkPipelineStageFlags       to,

                                                          VkAccessFlags              src_access,
                                                          VkAccessFlags              dst_access,

                                                          vc_queue_type              src_queue,
                                                          vc_queue_type              dst_queue,

                                                          VkImageSubresourceRange    subresource_range
                                                          );

void                vc_image_transition_layout(vc_ctx *ctx, vc_image image, VkImageLayout src_layout, VkImageLayout dst_layout, vc_queue_type queue, VkImageAspectFlags aspect);

/* ---------------- Buffers ---------------- */

/**
 * @brief Allocates a buffer
 *
 * @param ctx
 * @param alloc_desc The allocation parameters
 * @return vc_buffer A handle to the buffer
 */
vc_buffer           vc_buffer_allocate(vc_ctx *ctx, buffer_alloc_desc alloc_desc);

/**
 * @brief Makes a coherent write from CPU to buffer, this write is blocking on the CPU, which makes it not suitable for performant operations.
 *
 * @param ctx
 * @param dest Destination buffer
 * @param offset Offset in bytes in the buffer
 * @param length The length of the write
 * @param data The data to write
 * @param copy_queue The queue on which to send the copy command, and on which to wait
 */
void                vc_buffer_coherent_staged_write(vc_ctx *ctx, vc_buffer dest, u64 offset, u64 length, void *data, vc_queue_type copy_queue);

/**
 * @brief Simple copy into HOST_VISIBLE buffer, handles mapping, offseting, memcpy-ing, and unmapping the buffer, convinience function
 *
 * @param ctx
 * @param dest The destination buffer (needs to be HOST_VISIBLE, and allocation_flags need to be set accordingly)
 * @param offset The offset into the buffer from which to initiate the write
 * @param length The length of the write
 * @param data The data to write
 */
void                vc_buffer_write_to(vc_ctx *ctx, vc_buffer dest, u64 offset, u64 length, void *data);

/* ---------------- Images ---------------- */

/**
 * @brief Allocates an image
 *
 * @param ctx
 * @param desc Descripion of the image
 * @return vc_image A handle to the imaage
 */
vc_image            vc_image_allocate(vc_ctx *ctx, image_create_desc desc);

/**
 * @brief Creates the full image view for an image, that is, the image view viewing the full subRessources of the image (the one most commonly used)
 *
 * @param ctx
 * @param img The image on which to setup the full image view
 * @note The image object handle by @c{vc_image} contains a default image view (setup by this function)
 */
void                vc_image_create_full_image_view(vc_ctx *ctx, vc_image img);


vc_image_sampler    vc_image_sampler_create(vc_ctx *ctx, sampler_desc desc);
vc_image_view       vc_image_view_create(vc_ctx *ctx, vc_image image, image_view_desc desc);
void                vc_image_fill_from_buffer(vc_ctx *ctx, vc_image img, vc_buffer src, VkImageLayout transitioned_layout, VkImageAspectFlags aspect_dst, vc_queue_type queue);
/* ---------------- Graphics ---------------- */
/* ---------------- Render pass ---------------- */

/**
 * @brief Creates a render pass object
 *
 * @param ctx
 * @param desc
 * @return vc_render_pass
 */
vc_render_pass      vc_render_pass_create(vc_ctx *ctx, render_pass_desc desc);

/* ---------------- Framebuffer ---------------- */

/**
 * @brief Creates a frambuffer object
 *
 * @param ctx
 * @param desc
 * @return vc_framebuffer
 */
vc_framebuffer      vc_framebuffer_create(vc_ctx *ctx, framebuffer_desc desc);

/* ---------------- Graphics Pipeline ---------------- */

/**
 * @brief Creates a graphics pipeline
 *
 * @param ctx
 * @param desc
 * @return vc_graphics_pipe
 */
vc_graphics_pipe    vc_graphics_pipe_create(vc_ctx *ctx, graphics_pipeline_desc desc);

/* ---------------- Utils ---------------- */


/**
 * @brief Converts a set of vc_queue_flags to a list of queue indices according to the set bits
 *
 * @param ctx
 * @param flags The flags
 * @param[out] ids A pointer to a allocated emptly list of u32s
 */
void                vc_queue_flags_to_queue_indices_list(vc_ctx *ctx, vc_queue_flags flags, u32 *ids);

/**
 * @brief Returns the number of set bits in a flag
 *
 * @param flag The flag
 * @returns u32 The number of set bits in flag
 */
u32                 vc_u32_flags_set_bits(u32    flag);
