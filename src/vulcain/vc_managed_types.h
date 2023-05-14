#pragma once

#include "../base/base.h"
#include "vulcain.h"
#include <vulkan/vulkan.h>

typedef struct
{
    VkPipeline       pipeline;
    VkPipelineLayout layout;
} vc_priv_man_compute_pipe;

typedef struct
{
    VkCommandBuffer command_buffer;
    VkCommandPool   pool;
    vc_queue_type   queue_type;
} vc_priv_man_command_buffer;