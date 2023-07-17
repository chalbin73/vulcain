#include "base/fio.h"
#include "vulcain/vc_handles.h"
#include <vulkan/vulkan_core.h>
#define VC_ENABLE_WINDOWING_GLFW 1

#include "base/base.h"
#include "vulcain/vulcain.h"
#include <GLFW/glfw3.h>

GLFWwindow *window;

int    main(i32 argc, char **argv)
{
    INFO("/* ---------------- START ---------------- */");
    INFO("Hello, World ! Welcome to vulcain !");

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1920, 1080, "Hello !", NULL, NULL);
    glfwShowWindow(window);
    u32 exts_count    = 0;
    const char **exts = glfwGetRequiredInstanceExtensions(&exts_count);

    timer t = TIMER_START();

    vc_ctx ctx =
    {
        0
    };
    vc_create_ctx(
        &ctx,
        &(instance_desc){
            .app_name         = "Playground",
            .engine_name      = "Playground",
            .engine_version   = VK_MAKE_VERSION(0, 0, 0),
            .enable_debugging = TRUE,
            .extension_count  = exts_count,
            .extensions       = (char **)exts,
            .enable_windowing = TRUE,
            .windowing_system = vc_windowing_system_glfw(window),
        },
        &(physical_device_query){
            .allowed_types          = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU | VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
            .requested_features     = { .geometryShader = TRUE },
            .request_main_queue     = TRUE,
            .request_compute_queue  = TRUE,
            .request_transfer_queue = FALSE
        }
        );


    render_attachments_set render_att =
    {
        .attachment_count = 1,
        .attachments      = (vc_image[1]){ vc_swapchain_get_image_hndls(&ctx)[0] }
    };

    vc_render_pass pass = vc_render_pass_create(
        &ctx,
        (render_pass_desc)
        {
            .attachment_set = render_att,

            .attachment_desc = (render_pass_attachment_params[1])
            {
                [0] =
                {
                    .load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .store_op = VK_ATTACHMENT_STORE_OP_STORE,

                    .stencil_load_op  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE,

                    .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .final_layout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                }
            },

            .subpass_count  = 1,
            .subpasses_desc = (subpass_desc[1])
            {
                [0] =
                {
                    .pipline_type                 = VC_PIPELINE_TYPE_GRAPHICS,
                    .input_attachment_count       = 0,
                    .color_attachment_count       = 1,
                    .color_attachment_refs        = &(VkAttachmentReference){ .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
                    .preserve_attachment_count    = 0,
                    .depth_stencil_attachment_ref = NULL,
                }
            },

            .subpass_dependency_count = 1,
            .subpass_dependencies     = &(subpass_dependency_desc)
            {
                .src_id = VK_SUBPASS_EXTERNAL,
                .dst_id = 0,

                .src_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dst_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,

                .src_access = 0,
                .dst_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            }
        }
        );
    (void)pass;

    vc_framebuffer frambuffers[vc_swapchain_image_count(&ctx)];
    for(int i = 0; i < vc_swapchain_image_count(&ctx); i++)
    {
        render_attachments_set render_att =
        {
            .attachment_count = 1,
            .attachments      = (vc_image[1]){ vc_swapchain_get_image_hndls(&ctx)[i] }
        };

        frambuffers[i] = vc_framebuffer_create(
            &ctx,
            (framebuffer_desc)
            {
                .attachment_set = render_att,
                .render_pass    = pass,
                .layers         = 1,
            }
            );
    }

    graphics_pipeline_code_desc code;
    code.vertex_code          = fio_read_whole_file("shaders/a.vert.spv", &code.vertex_code_size);
    code.vertex_entry_point   = "main";
    code.fragment_code        = fio_read_whole_file("shaders/a.frag.spv", &code.fragment_code_size);
    code.fragment_entry_point = "main";


    vc_graphics_pipe graphics_pipe = vc_graphics_pipe_create(
        &ctx,
        (graphics_pipeline_desc)
        {
            .shader_code                = code,
            .set_layout_count           = 0,
            .render_pass                = pass,
            .subpass_index              = 0,
            .vertex_input_binding_count = 0,
            .topology                   = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .line_width                 = 1.0f,
            .viewport_scissor_count     = 1,
            .depth_test                 = FALSE,
            .stencil_test               = FALSE,
            .enable_depth_clamp         = FALSE,
            .enable_depth_bias          = FALSE,
            .sample_count               = VK_SAMPLE_COUNT_1_BIT,
            .sample_shading             = FALSE,
            .attchment_count            = 1,
            .attchment_blends           = &(VkPipelineColorBlendAttachmentState)
            {
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .colorBlendOp        = VK_BLEND_OP_ADD,
                .alphaBlendOp        = VK_BLEND_OP_ADD,
                .blendEnable         = VK_TRUE,
                .colorWriteMask      = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,
            },
            .blend_constants     = { 1, 1, 1, 1 },
            .dynamic_state_count = 2,
            .dynamic_states      = (VkDynamicState[2]){ VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT },
        }
        );
    (void)graphics_pipe;

    vc_command_buffer buf = vc_command_buffer_main_create(&ctx, VC_QUEUE_MAIN);

    vc_semaphore sem = vc_semaphore_create(&ctx);

    TIMER_LOG(t, "Vulkan init");

    while ( !glfwWindowShouldClose(window) )
    {
        vc_queue_wait_idle(&ctx, VC_QUEUE_COMPUTE);
        vc_queue_wait_idle(&ctx, VC_QUEUE_MAIN);
        u32 iid = 0;
        vc_swapchain_acquire_image(&ctx, &iid, sem);
        //vc_image curi = vc_swapchain_get_image_hndls(&ctx)[iid];

        vc_command_buffer_reset(&ctx, buf);
        vc_command_buffer_begin(&ctx, buf);

        vc_command_render_pass_begin(
            &ctx,
            buf,
            (render_pass_begin_desc){
                .pass              = pass,
                .render_area       = (VkRect2D){ { 0, 0 }, { 1916, 1036 } },
                .subpass_contents  = VK_SUBPASS_CONTENTS_INLINE,
                .clear_value_count = 1,
                .clear_values      = &(VkClearValue){ .color = { { 1, 1, 1, 1 } } },
                .frambuffer        = frambuffers[iid],
            }
            );

        vc_command_dyn_set_viewport(
            &ctx,
            buf,
            1,
            &(VkViewport){ .x = 0, .y = 0, .width = 1916, .height = 1036, .minDepth = 0.0f, .maxDepth = 1.0f }
            );

        vc_command_dyn_set_scissors(
            &ctx,
            buf,
            1,
            &(VkRect2D){ { 0, 0 }, { 1916, 1036 } }
            );

        vc_command_pipeline_bind(&ctx, buf, graphics_pipe);
        vc_command_draw(&ctx, buf, 3, 1, 0, 0);

        vc_command_render_pass_end(&ctx, buf);

        vc_command_buffer_end(&ctx, buf);
        vc_command_buffer_submit(
            &ctx,
            buf,
            sem,
            (VkPipelineStageFlags[1]){[0] = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT }
            );
        vc_queue_wait_idle(&ctx, VC_QUEUE_COMPUTE);

        vc_swapchain_present_image(&ctx, iid);

        glfwPollEvents();
    }
    vc_destroy_ctx(&ctx);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}