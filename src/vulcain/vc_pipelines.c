#include "vulcain.h"

vc_compute_pipe vc_compute_pipe_create(vc_ctx *ctx, compute_pipe_desc *desc)
{
    VkShaderModuleCreateInfo mod_ci = 
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = desc->shader_code_length,
        .pCode = (u32*)desc->shader_code,
    };

    return {0};
}

void            vc_compute_pipe_destroy(vc_ctx *ctx, vc_compute_pipe pipe)
{

}