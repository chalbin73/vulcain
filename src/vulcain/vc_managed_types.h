#pragma once

#include "../base/base.h"
#include <vulkan/vulkan.h>

typedef struct
{
    VkPipeline       pipeline;
    VkPipelineLayout layout;
} vc_priv_man_compute_pipe;

typedef struct
{
    VkCommandBuffer command_buffer;
} vc_priv_man_command_buffer;