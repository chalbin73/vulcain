#include "vc_managed_types.h"

b8                 _vc_priv_compute_pipe_destroy(vc_ctx *ctx, vc_priv_man_compute_pipe *pipe)
{
    vkDestroyPipeline(ctx->vk_device, pipe->pipeline, NULL);
    vkDestroyPipelineLayout(ctx->vk_device, pipe->layout, NULL);
    return TRUE;
}


b8                 _vc_priv_graphics_pipe_destroy(vc_ctx *ctx, vc_priv_man_graphics_pipe *pipe)
{
    vkDestroyPipeline(ctx->vk_device, pipe->pipeline, NULL);
    vkDestroyPipelineLayout(ctx->vk_device, pipe->layout, NULL);
    return TRUE;
}

vc_compute_pipe    vc_compute_pipe_create(vc_ctx *ctx, compute_pipe_desc *desc)
{
    VkShaderModuleCreateInfo mod_ci =
    {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = desc->shader_code_length,
        .pCode    = (u32 *)desc->shader_code,
    };

    VkShaderModule shad_mod;
    VK_CHECKH(vkCreateShaderModule(ctx->vk_device, &mod_ci, NULL, &shad_mod), "Could not create a compute shader module !");

    VkDescriptorSetLayout *layouts = alloca(sizeof(VkDescriptorSetLayout) * desc->set_layout_count);

    for(int i = 0; i < desc->set_layout_count; i++)
    {
        vc_priv_man_descriptor_set_layout *set_layout = vc_handle_mgr_ptr(&ctx->handle_manager, desc->set_layouts[i]);
        layouts[i] = set_layout->set_layout;
    }

    VkPipelineLayoutCreateInfo layout_ci =
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = desc->set_layout_count,
        .pSetLayouts            = layouts,
        .pushConstantRangeCount = 0,
    };

    VkPipelineLayout layout;
    VK_CHECKH(vkCreatePipelineLayout(ctx->vk_device, &layout_ci, NULL, &layout), "Could not create a comp pipe's layout.");

    VkComputePipelineCreateInfo comp_ci =
    {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage =
        {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shad_mod,
            .pName  = "main",
        },
        .layout     = layout,
    };

    VkPipeline pipeline;
    VK_CHECKH(vkCreateComputePipelines(ctx->vk_device, VK_NULL_HANDLE, 1, &comp_ci, NULL, &pipeline), "Could not create a compute pipeline.");

    vc_priv_man_compute_pipe pipe =
    {
        .type     = VC_PIPELINE_TYPE_COMPUTE,
        .pipeline = pipeline,
        .layout   = layout
    };

    vkDestroyShaderModule(ctx->vk_device, shad_mod, NULL);

    vc_compute_pipe pipe_h = vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_COMPUTE_PIPE, &pipe);
    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_COMPUTE_PIPE, (vc_man_destroy_func)_vc_priv_compute_pipe_destroy);
    return pipe_h;

}

vc_graphics_pipe    vc_graphics_pipe_create(vc_ctx *ctx, graphics_pipeline_desc desc)
{
    VkShaderModuleCreateInfo vert_ci =
    {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = desc.shader_code.vertex_code_size,
        .pCode    = (const u32 *)desc.shader_code.vertex_code,
    };

    VkShaderModuleCreateInfo frag_ci =
    {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = desc.shader_code.fragment_code_size,
        .pCode    = (const u32 *)desc.shader_code.fragment_code,
    };

    VkShaderModule vert_module;
    VkShaderModule frag_module;

    VK_CHECKH(vkCreateShaderModule(ctx->vk_device, &vert_ci, NULL, &vert_module), "Could not create a graphical pipline's vertex shader module.");
    VK_CHECKH(vkCreateShaderModule(ctx->vk_device, &frag_ci, NULL, &frag_module), "Could not create a graphical pipline's fragment shader module.");

    /* ---------------- INPUT STATE ---------------- */

    VkVertexInputBindingDescription *input_bindings = mem_allocate(sizeof(VkVertexInputBindingDescription) * desc.vertex_input_binding_count, MEMORY_TAG_RENDERER);
    u32 total_attributes                            = 0;

// Prepare bindings
    for(int i = 0; i < desc.vertex_input_binding_count; i++)
    {
        input_bindings[i] = (VkVertexInputBindingDescription)
        {
            .binding   = i,
            .stride    = desc.vertex_input_bindings[i].stride,
            .inputRate = desc.vertex_input_bindings[i].input_rate,
        };

        total_attributes += desc.vertex_input_bindings[i].attribute_count;
    }

    VkVertexInputAttributeDescription *input_attributes = mem_allocate(sizeof(VkVertexInputAttributeDescription) * total_attributes, MEMORY_TAG_RENDERER);
    u32 idx                                             = 0;

// Prepare attributes
    for(int i = 0; i < desc.vertex_input_binding_count; i++)
    {
        for(int j = 0; j < desc.vertex_input_bindings[i].attribute_count; j++)
        {
            input_attributes[idx++] = (VkVertexInputAttributeDescription)
            {
                .location = j,
                .binding  = i,
                .format   = desc.vertex_input_bindings[i].attributes[j].format,
                .offset   = desc.vertex_input_bindings[i].attributes[j].offset,
            };
        }
    }

    VkPipelineVertexInputStateCreateInfo vert_in_ci =
    {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,

        .vertexBindingDescriptionCount = desc.vertex_input_binding_count,
        .pVertexBindingDescriptions    = input_bindings,

        .vertexAttributeDescriptionCount = total_attributes,
        .pVertexAttributeDescriptions    = input_attributes,
    };

/* ---------------- Input assembly ---------------- */
    VkPipelineInputAssemblyStateCreateInfo assembly_ci =
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = desc.topology,
        .primitiveRestartEnable = VK_FALSE,
    };

/* ---------------- Tesselation ---------------- */
    VkPipelineTessellationStateCreateInfo tesselation_ci =
    {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .patchControlPoints = 0, // TODO: Expose that, as well as tesselation shader
    };

/* ---------------- Pipeline Viewport ---------------- */
    VkPipelineViewportStateCreateInfo viewport_ci =
    {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = desc.viewport_scissor_count,
        .scissorCount  = desc.viewport_scissor_count,
        .pViewports    = desc.viewports,
        .pScissors     = desc.scissors,
    };

/* ---------------- Rasterization ---------------- */
    VkPipelineRasterizationStateCreateInfo raster_ci =
    {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = (desc.enable_depth_clamp ? VK_TRUE : VK_FALSE),
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = desc.polygon_mode,
        .cullMode                = desc.cull_mode,
        .frontFace               = desc.front_face,
        .depthBiasEnable         = (desc.enable_depth_bias ? VK_TRUE : VK_FALSE),
        .depthBiasConstantFactor = desc.depth_bias_constant,
        .depthBiasClamp          = desc.depth_bias_clamp,
        .depthBiasSlopeFactor    = desc.depth_bias_slope,
        .lineWidth               = desc.line_width,
    };

/* ---------------- Multisample state ---------------- */
    VkPipelineMultisampleStateCreateInfo ms_ci =
    {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = desc.sample_count,
        .sampleShadingEnable  = (desc.sample_shading ? VK_TRUE : VK_FALSE),
        .minSampleShading     = desc.sample_shading_min_factor,

        // TODO: Investigate this (can be dynamic by the way)
        .pSampleMask          = NULL,
    };

/* ---------------- Depth/stencil ---------------- */
    VkPipelineDepthStencilStateCreateInfo depth_stencil_ci =
    {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,

        .depthTestEnable       = desc.depth_test,
        .depthWriteEnable      = desc.depth_write,
        .depthCompareOp        = desc.depth_compare_op,
        .depthBoundsTestEnable = desc.depth_bound_test_enable,

        .stencilTestEnable = desc.stencil_test,
        .front             = desc.front_faces_stencil_op,
        .back              = desc.back_faces_stencil_op,
        .minDepthBounds    = desc.depth_bounds_min,
        .maxDepthBounds    = desc.depth_bounds_max,
    };

    /* ---------------- Blend ---------------- */
    VkPipelineColorBlendStateCreateInfo blend_ci =
    {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable   = VK_FALSE,   //TODO: Investigate LogicOp
        .attachmentCount = desc.attchment_count,   //TODO: Maybe this can be infered from the render pass ?
        .pAttachments    = desc.attchment_blends,

        .blendConstants[0] = desc.blend_constants[0],
        .blendConstants[1] = desc.blend_constants[1],
        .blendConstants[2] = desc.blend_constants[2],
        .blendConstants[3] = desc.blend_constants[3],
    };

    /* ---------------- Dynamic ---------------- */
    VkPipelineDynamicStateCreateInfo dynamic_ci =
    {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = desc.dynamic_state_count,
        .pDynamicStates    = desc.dynamic_states,
    };

    /* ---------------- Layout ---------------- */
    VkPipelineLayoutCreateInfo layout_ci =
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = desc.set_layout_count,
        .pSetLayouts            = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL,
    };
    layout_ci.pSetLayouts = mem_allocate(sizeof(VkDescriptorSetLayout) * layout_ci.setLayoutCount, MEMORY_TAG_RENDERER);

    for(int i = 0; i < layout_ci.setLayoutCount; i++)
    {
        vc_priv_man_descriptor_set_layout *set_layout = vc_handle_mgr_ptr(&ctx->handle_manager, desc.set_layouts[i]);
        ( (VkDescriptorSetLayout *)layout_ci.pSetLayouts )[i] = set_layout->set_layout;
    }


    VkPipelineLayout pipe_layout;
    VK_CHECKH(vkCreatePipelineLayout(ctx->vk_device, &layout_ci, NULL, &pipe_layout), "Could not create a graphical pipline's layout.");

    mem_free( (void *)layout_ci.pSetLayouts );

    /* ---------------- Render pass ---------------- */
    vc_priv_man_render_pass *render_pass = vc_handle_mgr_ptr(&ctx->handle_manager, desc.render_pass);

    VkGraphicsPipelineCreateInfo graphics_ci =
    {
        .sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2, // TODO: Add support for geometry shader etc..
        .pStages    = (VkPipelineShaderStageCreateInfo[2])
        {
            [0] =
            {
                .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage               = VK_SHADER_STAGE_VERTEX_BIT,
                .module              = vert_module,
                .pName               = desc.shader_code.vertex_entry_point,
                .pSpecializationInfo = NULL,
            },
            [1] =
            {
                .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module              = frag_module,
                .pName               = desc.shader_code.fragment_entry_point,
                .pSpecializationInfo = NULL,
            },
        },
        .pVertexInputState   = &vert_in_ci,
        .pInputAssemblyState = &assembly_ci,
        .pTessellationState  = &tesselation_ci,
        .pViewportState      = &viewport_ci,
        .pRasterizationState = &raster_ci,
        .pMultisampleState   = &ms_ci,
        .pDepthStencilState  = &depth_stencil_ci,
        .pColorBlendState    = &blend_ci,
        .pDynamicState       = &dynamic_ci,
        .layout              = pipe_layout,
        .renderPass          = render_pass->render_pass,
        .subpass             = desc.subpass_index,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = 0,
    };

    VkPipeline pipeline;

    VK_CHECKH(vkCreateGraphicsPipelines(ctx->vk_device, VK_NULL_HANDLE, 1, &graphics_ci, NULL, &pipeline), "Could not create a graphics pipeline.");

    /* ---------------- Cleanup ---------------- */
    mem_free(input_attributes);
    mem_free(input_bindings);

    vkDestroyShaderModule(ctx->vk_device, vert_module, NULL);
    vkDestroyShaderModule(ctx->vk_device, frag_module, NULL);

    vc_priv_man_graphics_pipe man_pipeline =
    {
        .type     = VC_PIPELINE_TYPE_GRAPHICS,
        .pipeline = pipeline,
        .layout   = pipe_layout,
    };

    vc_graphics_pipe hndl = vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_GRAPHICS_PIPE, &man_pipeline);
    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_GRAPHICS_PIPE, (vc_man_destroy_func)_vc_priv_graphics_pipe_destroy);

    return hndl;
}

