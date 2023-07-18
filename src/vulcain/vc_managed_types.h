#pragma once

#include "../base/base.h"
#include "vulcain.h"
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

typedef struct
{
    // Struct header
    vc_pipeline_type    type;

    // Contents
    VkPipeline          pipeline;
    VkPipelineLayout    layout;
} vc_priv_man_compute_pipe;

typedef struct
{
    // Struct header
    vc_pipeline_type    type;

    // Contents
    VkPipeline          pipeline;
    VkPipelineLayout    layout;
} vc_priv_man_graphics_pipe;

typedef struct
{
    VkCommandBuffer    command_buffer;
    VkCommandPool      pool;
    vc_queue_type      queue_type;
} vc_priv_man_command_buffer;

typedef struct
{
    VkSemaphore    semaphore;
} vc_priv_man_semaphore;

typedef struct
{
    VkImage              image;
    VmaAllocation        allocation;

    b8                   external; // Wether or not the image is managed by an external system (e.g. swapchain)

    VkImageView          full_image_view; // Image view taking the full subressource range

    image_create_desc    image_desc;
} vc_priv_man_image;

typedef struct
{
    VkDescriptorSetLayout    set_layout;
    u64                      hash;
} vc_priv_man_descriptor_set_layout;

typedef struct
{
    u64                layout_hash;
    VkDescriptorSet    set;
} vc_priv_man_descriptor_set;

typedef struct
{
    VkBuffer                 buffer;
    VmaAllocation            allocation;
    VkMemoryPropertyFlags    memory_properties;
    u64                      size;
} vc_priv_man_buffer;

typedef struct
{
    VkRenderPass    render_pass;
} vc_priv_man_render_pass;

typedef struct
{
    VkFramebuffer    frambuffer;
} vc_priv_man_framebuffer;