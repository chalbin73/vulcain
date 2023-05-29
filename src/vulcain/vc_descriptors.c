#include "vc_managed_types.h"
#include "vulcain.h"

b8 _vc_descriptor_set_layout_destroy(vc_ctx *ctx, vc_priv_man_descriptor_set_layout *set_layout)
{
    vkDestroyDescriptorSetLayout(ctx->vk_device, set_layout->set_layout, NULL);
    return TRUE;
}

vc_descriptor_set_layout vc_descriptor_set_layout_create(vc_ctx *ctx)
{
}