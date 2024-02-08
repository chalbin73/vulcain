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

vc_compute_pipeline
vc_compute_pipeline_create(
    vc_ctx                     *ctx,

    u8                         *code,
    u64                         code_size,
    char                       *entry_point,

    u32                         set_layout_count,
    vc_descriptor_set_layout   *layouts,

    u32                         push_constants_count,
    VkPushConstantRange        *push_constants
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

    VkDescriptorSetLayout *set_layouts = alloca(sizeof(VkDescriptorSetLayout) * set_layout_count);
    for(u32 i = 0; i < set_layout_count; i++)
    {
        _vc_descriptor_set_layout_intern *sl_i = vc_handles_manager_deref(&ctx->handles_manager, layouts[i]);
        set_layouts[i] = sl_i->layout;
    }

    VkPipelineLayoutCreateInfo layout_ci =
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = set_layout_count,
        .pSetLayouts            = set_layouts,
        .pushConstantRangeCount = push_constants_count,
        .pPushConstantRanges    = push_constants,
    };

    _vc_compute_pipeline_intern comp_i =
    {
        0
    };

    VK_CHECKH(vkCreatePipelineLayout(ctx->current_device, &layout_ci, NULL, &comp_i.layout), "Could not create compute pipeline's layout.");


    VkComputePipelineCreateInfo comp_ci =
    {
        .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .flags              = NULL,
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

