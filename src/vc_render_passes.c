#include "vulcain.h"
#include "vc_managed_types.h"

b8                _vc_priv_render_pass_destroy(vc_ctx *ctx, vc_priv_man_render_pass *rp)
{
    vkDestroyRenderPass(ctx->vk_device, rp->render_pass, NULL);
    return TRUE;
}

vc_render_pass    vc_render_pass_create(vc_ctx *ctx, render_pass_desc desc)
{
    VkAttachmentDescription *attachments = mem_allocate(sizeof(VkAttachmentDescription) * desc.attachment_count, MEMORY_TAG_RENDERER);

    // Prepare attachment descriptions
    for(int i = 0; i < desc.attachment_count; i++)
    {
        attachments[i] = (VkAttachmentDescription)
        {
            .format  = desc.attachment_desc[i].attachment_format,
            .samples = desc.attachment_desc[i].attachment_sample_counts,

            .loadOp  = desc.attachment_desc[i].load_op,
            .storeOp = desc.attachment_desc[i].store_op,


            .stencilLoadOp  = desc.attachment_desc[i].stencil_load_op,
            .stencilStoreOp = desc.attachment_desc[i].stencil_store_op,

            .initialLayout = desc.attachment_desc[i].initial_layout,
            .finalLayout   = desc.attachment_desc[i].final_layout,
        };
    }

    // Prepare subpasses
    VkSubpassDescription *subpasses = mem_allocate(sizeof(VkSubpassDescription) * desc.subpass_count, MEMORY_TAG_RENDERER);
    for(int i = 0; i < desc.subpass_count; i++)
    {
        subpasses[i] = (VkSubpassDescription)
        {
            .pipelineBindPoint = vc_priv_pipeline_type_to_bind_point(desc.subpasses_desc[i].pipline_type),

            .inputAttachmentCount = desc.subpasses_desc[i].input_attachment_count,
            .pInputAttachments    = desc.subpasses_desc[i].input_attachment_refs,

            .colorAttachmentCount = desc.subpasses_desc[i].color_attachment_count,
            .pColorAttachments    = desc.subpasses_desc[i].color_attachment_refs,

            .pDepthStencilAttachment = desc.subpasses_desc[i].depth_stencil_attachment_ref,

            .preserveAttachmentCount = desc.subpasses_desc[i].preserve_attachment_count,
            .pPreserveAttachments    = desc.subpasses_desc[i].preserve_attachment_ids,

            // No resolve attachments for now
        };
    }

    // Prepare subpass dependencies
    VkSubpassDependency *dependencies = mem_allocate(sizeof(VkSubpassDependency) * desc.subpass_dependency_count, MEMORY_TAG_RENDERER);
    for(int i = 0; i < desc.subpass_dependency_count; i++)
    {
        dependencies[i] = (VkSubpassDependency)
        {
            .srcSubpass = desc.subpass_dependencies[i].src_id,
            .dstSubpass = desc.subpass_dependencies[i].dst_id,

            .srcStageMask = desc.subpass_dependencies[i].src_stages,
            .dstStageMask = desc.subpass_dependencies[i].dst_stages,

            .srcAccessMask = desc.subpass_dependencies[i].src_access,
            .dstAccessMask = desc.subpass_dependencies[i].dst_access,
        };
    }

    // Create render pass
    VkRenderPassCreateInfo render_pass_ci =
    {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,

        .attachmentCount = desc.attachment_count,
        .pAttachments    = attachments,

        .subpassCount = desc.subpass_count,
        .pSubpasses   = subpasses,

        .dependencyCount = desc.subpass_dependency_count,
        .pDependencies   = dependencies,
    };

    vc_priv_man_render_pass rp;
    VK_CHECKH(vkCreateRenderPass(ctx->vk_device, &render_pass_ci, NULL, &rp.render_pass), "Failed to create a render pass.");

    mem_free(attachments);
    mem_free(subpasses);
    mem_free(dependencies);

    vc_render_pass hndl = vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_RENDER_PASS, &rp);
    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_RENDER_PASS, (vc_man_destroy_func)_vc_priv_render_pass_destroy);

    return hndl;
}