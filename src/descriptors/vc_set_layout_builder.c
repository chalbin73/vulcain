#include "../base/base.h"
#include "../base/data_structures/darray.h"
#include "../vulcain.h"
#include "vc_set_layout_cache.h"
#include "../handles/vc_internal_types.h"

void
_vc_descriptor_set_layout_builder_init(vc_descriptor_set_layout_builder   *builder)
{
    builder->bindings = darray_create(VkDescriptorSetLayoutBinding);
}

void
vc_descriptor_set_layout_builder_add_binding(vc_descriptor_set_layout_builder *builder, u32 binding, VkDescriptorType type, VkShaderStageFlags stages)
{
    if(builder->bindings == NULL)
    {
        _vc_descriptor_set_layout_builder_init(builder);
    }

    VkDescriptorSetLayoutBinding sl_binding =
    {
        .binding            = binding,
        .stageFlags         = stages,
        .descriptorType     = type,
        .descriptorCount    = 1,
        .pImmutableSamplers = NULL,
    };

    darray_push(builder->bindings, sl_binding);
}

void
vc_descriptor_set_layout_builder_add_bindings(vc_descriptor_set_layout_builder *builder, u32 binding, u32 descriptor_count, VkDescriptorType type, VkShaderStageFlags stages)
{
    if(builder->bindings == NULL)
    {
        _vc_descriptor_set_layout_builder_init(builder);
    }

    VkDescriptorSetLayoutBinding sl_binding =
    {
        .binding            = binding,
        .stageFlags         = stages,
        .descriptorType     = type,
        .descriptorCount    = descriptor_count,
        .pImmutableSamplers = NULL,
    };

    darray_push(builder->bindings, sl_binding);
}

vc_descriptor_set_layout
vc_descriptor_set_layout_builder_build(vc_ctx *ctx, vc_descriptor_set_layout_builder *builder, VkDescriptorSetLayoutCreateFlags flags)
{
    VkDescriptorSetLayoutCreateInfo info =
    {
        0
    };

    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    info.bindingCount = darray_length(builder->bindings);
    info.pBindings    = builder->bindings;
    info.flags        = flags;
    info.pNext        = NULL;

    VkDescriptorSetLayout sl = vc_slc_get(&ctx->set_layout_cache, ctx->current_device, info);

    _vc_descriptor_set_layout_intern sl_i =
    {
        .layout = sl,
    };

    vc_descriptor_set_layout hndl = vc_handles_manager_walloc(&ctx->handles_manager, VC_HANDLE_DESCRIPTOR_SET_LAYOUT, &sl_i);

    darray_destroy(builder->bindings);
    *builder = (vc_descriptor_set_layout_builder) {
        0
    };

    return hndl;
}

