#include "vulcain.h"
#include "vc_managed_types.h"

b8                _vc_priv_framebuffer_destroy(vc_ctx *ctx, vc_priv_man_framebuffer *fb)
{
    vkDestroyFramebuffer(ctx->vk_device, fb->frambuffer, NULL);
    return TRUE;
}

vc_framebuffer    vc_framebuffer_create(vc_ctx *ctx, vc_framebuffer_desc desc)
{
    vc_priv_man_render_pass *pass = vc_handle_mgr_ptr(&ctx->handle_manager, desc.render_pass);

    VkFramebufferCreateInfo framebuffer_ci =
    {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = pass->render_pass,
        .attachmentCount = desc.attachment_set.attachment_count,

        .layers          = desc.layers // Still not sure 'bout that
    };

    vc_priv_man_image_view *img_view;
    vc_priv_man_image *img;
    framebuffer_ci.pAttachments = mem_allocate(sizeof(VkImageView) * desc.attachment_set.attachment_count, MEMORY_TAG_RENDERER);
    for(int i = 0; i < framebuffer_ci.attachmentCount; i++)
    {
        img_view = vc_handle_mgr_ptr(&ctx->handle_manager, desc.attachment_set.attachments[i]);
        img      = vc_handle_mgr_ptr(&ctx->handle_manager, img_view->parent_image);

        ( (VkImageView *)framebuffer_ci.pAttachments )[i] = img_view->image_view;

        // Infer frambuffer width and height from image sizes
        // Images can be smaller that the frambuffer's size
        // But when rendering, the render area must be smaller or equal to the smallest attachment
        framebuffer_ci.width  = MAX(img->image_desc.width, framebuffer_ci.width);
        framebuffer_ci.height = MAX(img->image_desc.height, framebuffer_ci.height);
    }

    vc_priv_man_framebuffer fb;
    VK_CHECKH(vkCreateFramebuffer(ctx->vk_device, &framebuffer_ci, NULL, &fb.frambuffer), "Could not create a framebuffer.");

    vc_framebuffer hndl = vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_FRAMEBUFFER, &fb);
    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_FRAMEBUFFER, (vc_man_destroy_func)_vc_priv_framebuffer_destroy);

    return hndl;
}
