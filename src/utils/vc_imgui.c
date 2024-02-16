#include "../vulcain.h"
#include "../handles/vc_handles.h"
#include "../handles/vc_internal_types.h"

#include "vc_imgui.h"

typedef struct
{
    VkDescriptorPool       imgui_pool;
    vc_windowing_system    win_sys;
} _vc_imgui_ctx;

void
vc_imgui_setup(vc_ctx *ctx, vc_queue gui_queue, vc_windowing_system windowing_system, VkFormat image_formats)
{
    igCreateContext(NULL);

    _vc_queue_intern *queue_i = vc_handles_manager_deref(&ctx->handles_manager, gui_queue);

    vc_trace("Setting up ImGui with '%s'.", windowing_system.windowing_system_name);

    ctx->imgui_ctx = mem_allocate(sizeof(_vc_imgui_ctx), MEMORY_TAG_RENDERER);
    _vc_imgui_ctx *i_ctx = ctx->imgui_ctx;
    i_ctx->win_sys = windowing_system;

    VkDescriptorPoolCreateInfo pool_info =
    {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = 1000,
        .poolSizeCount = 11,
        .pPoolSizes    = (VkDescriptorPoolSize[11]){ {
                                                         VK_DESCRIPTOR_TYPE_SAMPLER, 1000
                                                     },
                                                     {
                                                         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000
                                                     },
                                                     {
                                                         VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000
                                                     },
                                                     {
                                                         VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000
                                                     },
                                                     {
                                                         VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000
                                                     },
                                                     {
                                                         VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000
                                                     },
                                                     {
                                                         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000
                                                     },
                                                     {
                                                         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000
                                                     },
                                                     {
                                                         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000
                                                     },
                                                     {
                                                         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000
                                                     },
                                                     {
                                                         VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000
                                                     } }
    };

    vkCreateDescriptorPool(ctx->current_device, &pool_info, NULL, &i_ctx->imgui_pool);


    // Init for windowing system
    struct ImGui_ImplVulkan_InitInfo init_info =
    {
        .Instance              = ctx->vk_instance,
        .PhysicalDevice        = ctx->current_physical_device,
        .Device                = ctx->current_device,
        .Queue                 = queue_i->queue,
        .QueueFamily           = queue_i->queue_family_index,
        .DescriptorPool        = i_ctx->imgui_pool,
        .MinImageCount         = 3,
        .ImageCount            = 3,
        .UseDynamicRendering   = true,
        .ColorAttachmentFormat = image_formats,
        .MSAASamples           = VK_SAMPLE_COUNT_1_BIT,
    };

    windowing_system.ig_init(windowing_system.udata);
    ImGui_ImplVulkan_Init(&init_info, VK_NULL_HANDLE);

    ImGui_ImplVulkan_CreateFontsTexture();
}

void
vc_imgui_begin_frame(vc_ctx   *ctx)
{
    ImGui_ImplVulkan_NewFrame();
    _vc_imgui_ctx *i_ctx = ctx->imgui_ctx;
    i_ctx->win_sys.ig_begin_frame(ctx->windowing_system.udata);
    igNewFrame();
}

void
vc_cmd_imgui_end_frame_render(vc_cmd_record record, vc_image_view view, VkRect2D render_area, VkImageLayout layout)
{
    igRender();
    ImDrawData *draw_data          = igGetDrawData();
    _vc_command_buffer_intern *buf = (_vc_command_buffer_intern *)record;

    vc_cmd_begin_rendering(
        record,
        (vc_rendering_info)
        {
            .render_area             = render_area,
            .layer_count             = 1,
            .view_mask               = 0,
            .color_attachments_count = 1,
            .color_attachments       = &(vc_rendering_attachment_info)
            {
                .load_op      = VK_ATTACHMENT_LOAD_OP_LOAD,
                .store_op     = VK_ATTACHMENT_STORE_OP_STORE,
                .image_view   = view,
                .image_layout = layout,
            }
        }
        );

    ImGui_ImplVulkan_RenderDrawData(draw_data, buf->buffer, VK_NULL_HANDLE);

    vc_cmd_end_rendering(record);
}

void
vc_imgui_cleanup(vc_ctx   *ctx)
{

    ImGui_ImplVulkan_Shutdown();
    _vc_imgui_ctx *i_ctx = ctx->imgui_ctx;
    vkDestroyDescriptorPool(ctx->current_device, i_ctx->imgui_pool, NULL);
    i_ctx->win_sys.ig_shutdown(i_ctx->win_sys.udata);
    mem_free(ctx->imgui_ctx);
}

