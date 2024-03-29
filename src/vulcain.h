#ifndef __VULCAIN_H__
#define __VULCAIN_H__

#include "vc_windowing.h"
#include "handles/vc_handles.h"
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include <stdint.h>

// ## TODO: Make those header private
#include <vk_mem_alloc.h>
#include "descriptors/vc_ds_alloc.h"
#include "descriptors/vc_set_layout_cache.h"
// ##

#include "femtolog.h"

#define vc_trace(fmt, ...) \
        fl_log(TRACE, __FILE__, __LINE__, fmt, ## __VA_ARGS__);

#define vc_debug(fmt, ...) \
        fl_log(DEBUG, __FILE__, __LINE__, fmt, ## __VA_ARGS__);

#define vc_info(fmt, ...) \
        fl_log(INFO, __FILE__, __LINE__, fmt, ## __VA_ARGS__);

#define vc_warn(fmt, ...) \
        fl_log(WARN, __FILE__, __LINE__, fmt, ## __VA_ARGS__);

#define vc_error(fmt, ...) \
        fl_log(ERROR, __FILE__, __LINE__, fmt, ## __VA_ARGS__);

#define vc_fatal(fmt, ...) \
        fl_log(FATAL, __FILE__, __LINE__, fmt, ## __VA_ARGS__);

// Represents various features which need to be checked before use.
typedef struct
{
    b8    dynamic_rendering;
} vc_ctx_supported_features;

// Welcome to vulcain
typedef struct
{
    vc_handles_manager             handles_manager;

    bool                           windowing_enabled; // This means that the application runs in some sort of a window, so swapchains can be created.
    bool                           debugging_enabled; // This is true if some validation layers are requested.

    vc_windowing_system            windowing_system; // In the case a windowing system is being used.

    VkInstance                     vk_instance;
    VkDebugUtilsMessengerEXT       debugging_messenger; // Only used if debugging_enabled.

    VkPhysicalDevice               current_physical_device;
    VkDevice                       current_device;

    VmaAllocator                   main_allocator; // See if it would be a good idea to allow multiple allocators ...

    // TODO: Make those two invisible to the outside world
    vc_descriptor_set_allocator    ds_allocator;
    vc_set_layout_cache            set_layout_cache;

    vc_ctx_supported_features      supported_features;

    // Optional features
    void                          *imgui_ctx;
} vc_ctx;

typedef struct
{
    uint32_t    format_count;
    VkFormat   *formats;
} vc_format_set;

typedef struct
{
    VkFormatFeatureFlags    required_linear_tiling_features;
    VkFormatFeatureFlags    required_optimal_tiling_features;
    VkFormatFeatureFlags    required_buffer_features;
} vc_format_query;

// ## VC_CTX ##

bool vc_ctx_create(vc_ctx                *ctx,
                   VkApplicationInfo      app_info,
                   vc_windowing_system   *windowing_system,
                   bool                   enable_debugging,
                   uint32_t               layer_count,
                   const char           **layer_names,
                   uint32_t               extension_count,
                   const char           **extension_names);

void     vc_ctx_destroy(vc_ctx   *ctx);

void     vc_queue_wait_idle(vc_ctx *ctx, vc_queue queue);
void     vc_device_wait_idle(vc_ctx   *ctx);

// ## FORMAT UTILS ##

VkFormat vc_format_query_format(vc_ctx *ctx, vc_format_query query, vc_format_set candidates);
b8       vc_format_query_index(vc_ctx *ctx, vc_format_query query, vc_format_set candidates, u32 *index);

// ## SWAPCHAIN ##

/*
 * @brief Describes the swapchain parameters, after a swapchain has been created
 */
typedef struct
{
    VkExtent2D       swapchain_extent;
    VkFormat         swapchain_image_format;
    vc_swapchain     swapchain;

    uint32_t         swapchain_image_count;
    vc_image        *images;
    vc_image_view   *image_views;

} vc_swapchain_created_info;

typedef void (*vc_swapchain_callback_func)(vc_ctx *ctx, void *udata, vc_swapchain_created_info);

typedef u32 vc_swpchn_img_id;

vc_swapchain vc_swapchain_create(vc_ctx                       *ctx,
                                 vc_windowing_system           win_sys,
                                 VkImageUsageFlags             image_usage,
                                 vc_format_query               query,
                                 vc_swapchain_callback_func    create_clbk,
                                 vc_swapchain_callback_func    destroy_clbk,
                                 void                         *clbk_udata);

void              vc_swapchain_present_image(vc_ctx *ctx, vc_swapchain swapchain, vc_queue presentation_queue, vc_semaphore wait_semaphore, vc_swpchn_img_id image_id);
vc_swpchn_img_id  vc_swapchain_acquire_image(vc_ctx *ctx, vc_swapchain swapchain, vc_semaphore *signal_semaphore);
vc_image          vc_swapchain_get_image(vc_ctx *ctx, vc_swapchain swapchain, vc_swpchn_img_id index);
void              vc_handle_destroy(vc_ctx *ctx, vc_handle hndl);
void              vc_swapchain_get_info(vc_ctx *ctx, vc_swapchain swapchain, vc_swapchain_created_info *info_out);
void              vc_swapchain_present_images(vc_ctx *ctx, u32 swapchain_count, vc_swapchain *swapchains, vc_swpchn_img_id *image_ids, vc_queue presentation_queue, u32 wait_semaphore_count, vc_semaphore *wait_semaphores);

// ## COMMAND BUFFERS/POOLS ##

/**
 * @brief Creates a command pool
 *
 * @param ctx The vulcain context
 * @param parent_queue A queue of the queue family to create the command pool with
 * @param flags The flags with which to create te command pool
 * @return A handle to a command pool
 */
vc_command_pool   vc_command_pool_create(vc_ctx *ctx, vc_queue parent_queue, VkCommandPoolCreateFlags flags);

/**
 * @brief Allocates a command buffer
 *
 * @param ctx The vulcain context
 * @param level The level of the command buffer
 * @param pool The pool in which to allocate the command buffer
 * @return A handle to a command buffer
 */
vc_command_buffer vc_command_buffer_allocate(vc_ctx *ctx, VkCommandBufferLevel level, vc_command_pool pool);

// ## SYNCHRONISATOIN OBJECTS ##

vc_semaphore      vc_semaphore_create(vc_ctx   *ctx);

// ## IMAGES ##

typedef struct
{
    VmaMemoryUsage              usage;
    VkMemoryPropertyFlags       mem_props;
    VmaAllocationCreateFlags    flags;
} vc_memory_create_info;

typedef struct
{
    uint8_t                  image_dimension;
    VkFormat                 image_format;
    uint32_t                 width;
    uint32_t                 height;
    uint32_t                 depth;

    uint32_t                 mip_level_count;
    uint32_t                 array_layer_count;

    VkSampleCountFlagBits    sample_count;
    VkImageTiling            tiling;
    VkImageUsageFlags        usage;

    b8                       sharing_exclusive;
    vc_queue                *queues;
    uint32_t                 queue_count;

    VkImageLayout            initial_layout;
    vc_memory_create_info    memory;
} vc_image_create_info;

vc_image      vc_image_allocate(vc_ctx *ctx, vc_image_create_info create_info);
vc_image_view vc_image_view_create(vc_ctx *ctx, vc_image image, VkImageViewType type, VkComponentMapping component_map, VkImageSubresourceRange range);

// Useful utils
#define VC_COMP_MAP_ID \
        (VkComponentMapping) \
        { \
            .a = VK_COMPONENT_SWIZZLE_A, \
            .r = VK_COMPONENT_SWIZZLE_R, \
            .g = VK_COMPONENT_SWIZZLE_G, \
            .b = VK_COMPONENT_SWIZZLE_B, \
        }

#define VC_IMG_SUBRES_COLOR_1 \
        (VkImageSubresourceRange) \
        { \
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT, \
            .layerCount     = 1, \
            .levelCount     = 1, \
            .baseMipLevel   = 0, \
            .baseArrayLayer = 0, \
        }

// ## BUFFERS ##

/**
 * @brief Allocates a buffer
 *
 * @param ctx A vulcain context
 * @param size The size of the buffer to allocate
 * @param flags The flags with which to create the buffer
 * @param usage The usage of the buffer
 * @param mem The memory information about the allocation
 * @return A handle to the buffer
 */
vc_buffer vc_buffer_allocate(vc_ctx *ctx, u64 size, VkBufferCreateFlags flags, VkBufferUsageFlags usage, vc_memory_create_info mem);

// ## DESCRIPTORS ##

// Set layouts

typedef struct
{
    VkDescriptorSetLayoutBinding   *bindings; // darray
} vc_descriptor_set_layout_builder;

/**
 * @brief Adds a single descriptor binding to descriptor set layout
 *
 * @param builder The builder (may be 0 initialized)
 * @param binding The binding index
 * @param type The descriptor type
 * @param stages The stages
 */
void                     vc_descriptor_set_layout_builder_add_binding(vc_descriptor_set_layout_builder *builder, u32 binding, VkDescriptorType type, VkShaderStageFlags stages);
/**
 * @brief Adds some descriptors to a descriptor set layout
 *
 * @param builder The builder (may be 0 initialized)
 * @param binding The binding index
 * @param descriptor_count The number of descriptors
 * @param type The descriptor type
 * @param stages The stages
 */
void                     vc_descriptor_set_layout_builder_add_bindings(vc_descriptor_set_layout_builder *builder, u32 binding, u32 descriptor_count, VkDescriptorType type, VkShaderStageFlags stages);


/**
 * @brief Builds a descriptor set
 *
 * @param ctx A vulcain context
 * @param builder The (non non-inited) builder
 * @param flags The flags to create the set layout with
 * @return A handle to the set layout
 */
vc_descriptor_set_layout vc_descriptor_set_layout_builder_build(vc_ctx *ctx, vc_descriptor_set_layout_builder *builder, VkDescriptorSetLayoutCreateFlags flags);

// Descriptor sets

/**
 * @brief Allocates a descriptor in the internal pool system
 *
 * @param ctx The vulcain context
 * @param layout The set layout with which to create the descriptor
 * @return A handle to the allocated descriptor set
 */
vc_descriptor_set        vc_descriptor_set_allocate(vc_ctx *ctx, vc_descriptor_set_layout layout);

/**
 * @brief Representes a writer, which helps writing into descriptor sets
 */
typedef struct
{
    // darrays
    VkWriteDescriptorSet     *writes;

    VkDescriptorImageInfo    *img_infos;
    VkDescriptorBufferInfo   *buf_infos;
} vc_descriptor_set_writer;

/**
 * @brief Writes a buffer type descriptor into the descriptor set
 *
 * @param ctx A vulcain context
 * @param writer The writer
 * @param binding The destination binding
 * @param array_elt The destination array element
 * @param buffer The buffer handle
 * @param offset The offset in device units into the buffer
 * @param range The range in device units into the buffer
 * @param buffer_type The precise type of buffer descriptor
 */
void vc_descriptor_set_writer_write_buffer(vc_ctx *ctx, vc_descriptor_set_writer *writer, u32 binding, u32 array_elt, vc_handle buffer, u64 offset, u64 range, VkDescriptorType buffer_type);

/**
 * @brief Writes an image type descriptor into the descriptor set
 *
 * @param ctx A vulcain context
 * @param writer The writer
 * @param binding The destination binding
 * @param array_elt The destination array element
 * @param view The image view (Can be VC_NULL_HANDLE)
 * @param sampler A sampler (Can be VC_NULL_HANDLE)
 * @param layout The layout in which the image will be when accessed/sampled
 * @param image_type The precise type of image descriptor
 */
void vc_descriptor_set_writer_write_image(vc_ctx *ctx, vc_descriptor_set_writer *writer, u32 binding, u32 array_elt, vc_handle view, vc_handle sampler, VkImageLayout layout, VkDescriptorType image_type);


/**
 * @brief Updates the descriptor set with the written information
 *
 * @param ctx A vulcain context
 * @param writer The writer
 * @param set The destination set
 */
void vc_descriptor_set_writer_write(vc_ctx *ctx, vc_descriptor_set_writer *writer, vc_descriptor_set set);


// ## PIPELINES ##

typedef struct
{
    u32                         set_layout_count;
    vc_descriptor_set_layout   *set_layouts;

    u32                         push_constants_count;
    VkPushConstantRange        *push_constants;
} vc_pipeline_layout_info;

typedef struct
{
    u8           *vertex_code;
    u64           vertex_code_size;
    const char   *vertex_entry_point;

    u8           *fragment_code;
    u64           fragment_code_size;
    const char   *fragment_entry_point;
} vc_gfx_pipeline_code_info;

// - Vertex bindings
typedef struct
{
    u32         location;
    VkFormat    format;
    u32         offset;
} vc_vertex_binding_attribute;

typedef struct
{
    u32                            binding;
    u32                            stride;
    u32                            attribute_count;
    vc_vertex_binding_attribute   *attributes;

    VkVertexInputRate              input_rate;
} vc_vertex_binding;

typedef struct
{

    /** @brief The code of the programmable pipeline stages */
    vc_gfx_pipeline_code_info              shader_code;

    vc_pipeline_layout_info                layout_info;

    // Assembly state
    u32                                    vertex_binding_count;
    vc_vertex_binding                     *vertex_bindings;

    VkPrimitiveTopology                    topology;

    // Ignored if dynamic
    u32                                    viewport_scissor_count;
    VkViewport                            *viewports;
    VkRect2D                              *scissors;

    // Depth state
    b8                                     depth_test;
    b8                                     depth_write;
    VkCompareOp                            depth_compare_op;
    b8                                     depth_bound_test_enable;
    f32                                    depth_bounds_min;
    f32                                    depth_bounds_max;

    // Stencil test
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

    // Multisampling
    VkSampleCountFlagBits                  sample_count;
    b8                                     sample_shading;
    f32                                    sample_shading_min_factor;

    // Attachment state
    u32                                    attachment_count;
    VkPipelineColorBlendAttachmentState   *attachment_blends;
    f32                                    blend_constants[4];

    // Dynamic states
    u32                                    dynamic_state_count;
    VkDynamicState                        *dynamic_states;

} vc_graphics_pipeline_desc;

// For dynamic rendering
typedef struct
{
    uint32_t    view_mask;
    uint32_t    color_attachment_count;
    VkFormat   *color_attachment_formats;
    VkFormat    depth_attachment_format;
    VkFormat    stencil_attachment_format;
} vc_pipeline_rendering_info;

vc_gfx_pipeline vc_gfx_pipeline_dynamic_create(
    vc_ctx                       *ctx,
    vc_graphics_pipeline_desc     desc,
    vc_pipeline_rendering_info    dyn_info
    );

vc_compute_pipeline vc_compute_pipeline_create(
    vc_ctx                    *ctx,

    u8                        *code,
    u64                        code_size,
    char                      *entry_point,

    vc_pipeline_layout_info    layout_info
    );

typedef enum
{
    VC_PIPELINE_COMPUTE = 1,
    VC_PIPELINE_GRAPHICS,
    VC_PIPELINE_TYPE_MAX,
} vc_pipeline_type;

// ## DYNAMIC RENDERING ##

/**
 * @brief Represents an opaque "Command buffer recording context". The purpose is to accelerate frequent accesses to the same object in
 *        performance sensible operations.
 */

typedef struct
{
    vc_image_view            image_view;
    VkImageLayout            image_layout;
    VkResolveModeFlagBits    resolve_mode;
    vc_image_view            resolve_image_view;
    VkImageLayout            resolve_image_layout;
    VkAttachmentLoadOp       load_op;
    VkAttachmentStoreOp      store_op;
    VkClearValue             clear_value;
} vc_rendering_attachment_info;

typedef struct
{
    VkRenderingFlags                flags;
    VkRect2D                        render_area;
    u32                             layer_count;
    u32                             view_mask;
    u32                             color_attachments_count;
    vc_rendering_attachment_info   *color_attachments;
    vc_rendering_attachment_info   *depth_attachment;
    vc_rendering_attachment_info   *stencil_attachment;
} vc_rendering_info;


// ## COMMAND BUFFERS ##
typedef uint64_t vc_cmd_record;

void          vc_command_buffer_submit(vc_ctx *ctx, vc_command_buffer buffer, vc_queue queue_submit,
                                       u32 wait_sem_count, vc_semaphore *wait_sems, VkPipelineStageFlags *wait_stages,
                                       u32 signal_sem_count, vc_semaphore *signal_sems);

void          vc_command_buffer_end(vc_cmd_record    record);

vc_cmd_record vc_command_buffer_begin(vc_ctx *ctx, vc_command_buffer cmd_buffer, VkCommandBufferUsageFlags usage);

void          vc_cmd_image_barrier(vc_cmd_record record, vc_image image,
                                   VkPipelineStageFlags src_stages, VkPipelineStageFlags dst_stages,
                                   VkAccessFlags src_access, VkAccessFlags dst_access,
                                   VkImageLayout old_layout, VkImageLayout new_layout,
                                   VkImageSubresourceRange subres_range,
                                   vc_queue src_queue, vc_queue dst_queue
                                   );

void vc_cmd_image_clear(vc_cmd_record record, vc_image image,
                        VkImageLayout layout,
                        VkClearColorValue clear_color,
                        VkImageSubresourceRange subres_range);


void vc_cmd_bind_descriptor_set(vc_cmd_record record, vc_handle pipeline, vc_descriptor_set set, u32 set_dest);
void vc_cmd_dispatch_compute(vc_cmd_record record, vc_compute_pipeline pipeline, u32 groups_x, u32 groups_y, u32 groups_z);
void vc_cmd_push_constants(vc_cmd_record record, vc_handle pipeline, VkShaderStageFlags stage, u32 offset, u32 size, void *data);

void vc_cmd_draw(vc_cmd_record record, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
void vc_cmd_bind_pipeline(vc_cmd_record record, vc_gfx_pipeline pipeline);

// dynamic rendering
void vc_cmd_begin_rendering(vc_cmd_record record, vc_rendering_info info);
void vc_cmd_end_rendering(vc_cmd_record    record);

// ## IMGUI ##
void vc_imgui_setup(vc_ctx *ctx, vc_queue gui_queue, vc_windowing_system windowing_system, VkFormat image_formats);

void vc_imgui_cleanup(vc_ctx   *ctx);
void vc_cmd_imgui_end_frame_render(vc_cmd_record record, vc_image_view view, VkRect2D render_area, VkImageLayout layout);
void vc_imgui_begin_frame(vc_ctx   *ctx);
#endif //__VULCAIN_H__

