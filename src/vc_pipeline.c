#include "vulcain.h"
#include "handles/vc_internal_types.h"
#include "vc_enum_util.h"
#include <alloca.h>

void
_vc_compute_pipeline_destroy(vc_ctx *ctx, _vc_compute_pipeline_intern *c)
{
    vkDestroyPipelineLayout(ctx->current_device, c->layout, NULL);
    vkDestroyPipeline(ctx->current_device, c->pipeline, NULL);
}

void
_vc_gfx_pipeline_destroy(vc_ctx *ctx, _vc_gfx_pipeline_intern *c)
{
    vkDestroyPipelineLayout(ctx->current_device, c->layout, NULL);
    vkDestroyPipeline(ctx->current_device, c->pipeline, NULL);
}

VkPipelineLayout
_vc_pipeline_layout_create(vc_ctx *ctx, vc_pipeline_layout_info layout_info)
{
    VkPipelineLayout layout            = VK_NULL_HANDLE;
    VkDescriptorSetLayout *set_layouts = alloca(sizeof(VkDescriptorSetLayout) * layout_info.set_layout_count);
    for(u32 i = 0; i < layout_info.set_layout_count; i++)
    {
        _vc_descriptor_set_layout_intern *sl_i = vc_handles_manager_deref(&ctx->handles_manager, layout_info.set_layouts[i]);
        set_layouts[i] = sl_i->layout;
    }

    VkPipelineLayoutCreateInfo layout_ci =
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = layout_info.set_layout_count,
        .pSetLayouts            = set_layouts,
        .pushConstantRangeCount = layout_info.push_constants_count,
        .pPushConstantRanges    = layout_info.push_constants,
    };

    VK_CHECKH(vkCreatePipelineLayout(ctx->current_device, &layout_ci, NULL, &layout), "Could not create a pipeline's layout.");
    return layout;
}

vc_compute_pipeline
vc_compute_pipeline_create(
    vc_ctx                    *ctx,

    u8                        *code,
    u64                        code_size,
    char                      *entry_point,

    vc_pipeline_layout_info    layout_info
    )
{
    // Shader stage

    VkShaderModuleCreateInfo mod_ci =
    {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pCode    = (const u32 *)code,
        .codeSize = code_size,
    };

    VkShaderModule comp_module = VK_NULL_HANDLE;
    VK_CHECKH(vkCreateShaderModule(ctx->current_device, &mod_ci, NULL, &comp_module), "Could not create shader module.");

    VkPipelineShaderStageCreateInfo comp_stage =
    {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage               = VK_SHADER_STAGE_COMPUTE_BIT,
        .flags               = 0, // TODO: Might want to support flags here,
        .module              = comp_module,
        .pName               = entry_point,
        .pSpecializationInfo = NULL,
    };

    // PIPELINE LAYOUT

    _vc_compute_pipeline_intern comp_i =
    {
        0
    };

    comp_i.layout = _vc_pipeline_layout_create(ctx, layout_info);

    VkComputePipelineCreateInfo comp_ci =
    {
        .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .flags              = 0,
        .layout             = comp_i.layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .stage              = comp_stage,
    };

    VK_CHECKH(vkCreateComputePipelines(ctx->current_device, VK_NULL_HANDLE, 1, &comp_ci, NULL, &comp_i.pipeline), "Could not create compute pipeline.");
    vkDestroyShaderModule(ctx->current_device, comp_module, NULL);

    comp_i.type = VC_PIPELINE_COMPUTE;

    vc_compute_pipeline hndl = vc_handles_manager_walloc(&ctx->handles_manager, VC_HANDLE_COMPUTE_PIPELINE, &comp_i);
    vc_handles_manager_set_destroy_function(&ctx->handles_manager, VC_HANDLE_COMPUTE_PIPELINE, (vc_handle_destroy_func)_vc_compute_pipeline_destroy);

    return hndl;
}

vc_gfx_pipeline
vc_gfx_pipeline_dynamic_create(
    vc_ctx                       *ctx,
    vc_graphics_pipeline_desc     desc,
    vc_pipeline_rendering_info    dyn_info
    )
{
    // ## SHADER MODULES
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

    VK_CHECKH(vkCreateShaderModule(ctx->current_device, &vert_ci, NULL, &vert_module), "Could not create a graphical pipline's vertex shader module.");
    VK_CHECKH(vkCreateShaderModule(ctx->current_device, &frag_ci, NULL, &frag_module), "Could not create a graphical pipline's fragment shader module.");

    /* ---------------- INPUT STATE ---------------- */
    VkVertexInputBindingDescription *input_bindings = alloca(sizeof(VkVertexInputBindingDescription) * desc.vertex_binding_count);
    u32 total_attributes                            = 0;

    // Prepare bindings
    for(int i = 0; i < desc.vertex_binding_count; i++)
    {
        input_bindings[i] = (VkVertexInputBindingDescription)
        {
            .binding   = desc.vertex_bindings[i].binding,
            .stride    = desc.vertex_bindings[i].stride,
            .inputRate = desc.vertex_bindings[i].input_rate,
        };

        total_attributes += desc.vertex_bindings[i].attribute_count;
    }

    VkVertexInputAttributeDescription *input_attributes = alloca(sizeof(VkVertexInputAttributeDescription) * total_attributes);
    u32 idx                                             = 0;

    // Prepare attributes
    for(int i = 0; i < desc.vertex_binding_count; i++)
    {
        for(int j = 0; j < desc.vertex_bindings[i].attribute_count; j++)
        {
            input_attributes[idx++] = (VkVertexInputAttributeDescription)
            {
                .location = desc.vertex_bindings[i].attributes[j].location,
                .binding  = desc.vertex_bindings[i].binding,
                .format   = desc.vertex_bindings[i].attributes[j].format,
                .offset   = desc.vertex_bindings[i].attributes[j].offset,
            };
        }
    }

    VkPipelineVertexInputStateCreateInfo vert_in_ci =
    {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,

        .vertexBindingDescriptionCount = desc.vertex_binding_count,
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
        .attachmentCount = desc.attachment_count,   //TODO: Maybe this can be infered from the render pass ?
        .pAttachments    = desc.attachment_blends,

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



    /* ---------------- Pipeline layout ---------------- */
    VkPipelineLayout pipe_layout = _vc_pipeline_layout_create(ctx, desc.layout_info);

    /* ---------------- Rendering info ---------------- */
    VkPipelineRenderingCreateInfo rendering_info_ci =
    {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .viewMask                = dyn_info.view_mask,
        .colorAttachmentCount    = dyn_info.color_attachment_count,
        .pColorAttachmentFormats = (VkFormat *)dyn_info.color_attachment_formats,
        .depthAttachmentFormat   = dyn_info.depth_attachment_format,
        .stencilAttachmentFormat = dyn_info.stencil_attachment_format,
    };

    VkGraphicsPipelineCreateInfo graphics_ci =
    {
        .sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext      = &rendering_info_ci,
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
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = 0,
    };

    VkPipeline pipeline = VK_NULL_HANDLE;
    VK_CHECKH(vkCreateGraphicsPipelines(ctx->current_device, VK_NULL_HANDLE, 1, &graphics_ci, NULL, &pipeline), "Could not create a graphics pipeline.");

    /* ---------------- Cleanup ---------------- */

    vkDestroyShaderModule(ctx->current_device, vert_module, NULL);
    vkDestroyShaderModule(ctx->current_device, frag_module, NULL);

    _vc_gfx_pipeline_intern pipe_i =
    {
        .layout   = pipe_layout,
        .pipeline = pipeline,
        .type     = VC_PIPELINE_GRAPHICS,
    };

    vc_gfx_pipeline hndl = vc_handles_manager_walloc(&ctx->handles_manager, VC_HANDLE_GFX_PIPELINE, &pipe_i);
    vc_handles_manager_set_destroy_function(&ctx->handles_manager, VC_HANDLE_GFX_PIPELINE, (vc_handle_destroy_func)_vc_gfx_pipeline_destroy);
    return hndl;
}

