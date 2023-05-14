#include "vc_managed_types.h"

b8 _vc_priv_compute_pipe_destroy(vc_ctx *ctx, vc_priv_man_compute_pipe *pipe);

vc_compute_pipe vc_compute_pipe_create(vc_ctx *ctx, compute_pipe_desc *desc)
{
    VkShaderModuleCreateInfo mod_ci =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = desc->shader_code_length,
            .pCode = (u32 *)desc->shader_code,
        };

    VkShaderModule shad_mod;
    VK_CHECKH(vkCreateShaderModule(ctx->vk_device, &mod_ci, NULL, &shad_mod), "Could not create a compute shader module !");

    VkPipelineLayoutCreateInfo layout_ci =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 0,
            .pushConstantRangeCount = 0,
        };

    VkPipelineLayout layout;
    VK_CHECKH(vkCreatePipelineLayout(ctx->vk_device, &layout_ci, NULL, &layout), "Could not create a comp pipe's layout.");

    VkComputePipelineCreateInfo comp_ci =
        {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                        .module = shad_mod,
                        .pName = "main",
                        },
            .layout = layout,
    };

    VkPipeline pipeline;
    VK_CHECKH(vkCreateComputePipelines(ctx->vk_device, VK_NULL_HANDLE, 1, &comp_ci, NULL, &pipeline), "Could not create a compute pipeline.");

    vc_priv_man_compute_pipe pipe =
        {
            .pipeline = pipeline,
            .layout = layout};

    vkDestroyShaderModule(ctx->vk_device, shad_mod, NULL);

    vc_compute_pipe pipe_h = vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_COMPUTE_PIPE, &pipe);
    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_COMPUTE_PIPE, (vc_man_destroy_func)_vc_priv_compute_pipe_destroy);
    return pipe_h;
}

b8 _vc_priv_compute_pipe_destroy(vc_ctx *ctx, vc_priv_man_compute_pipe *pipe)
{
    vkDestroyPipeline(ctx->vk_device, pipe->pipeline, NULL);
    vkDestroyPipelineLayout(ctx->vk_device, pipe->layout, NULL);
    return TRUE;
}