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
    INFO("Hello, World ! Welcome to vulcain !");

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1920, 1080, "Hello !", NULL, NULL);
    glfwShowWindow(window);
    u32 exts_count    = 0;
    const char **exts = glfwGetRequiredInstanceExtensions(&exts_count);

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

    //    vkGetPhysicalDeviceFormatProperties(ctx.vk_selected_physical_device, VK_FORMAT_A2B10G10R10_SNORM_PACK32, VkFormatProperties *pFormatProperties)

    // vc_buffer buffer = vc_buffer_allocate(
    //     &ctx,
    //     (buffer_alloc_desc){
    //         .buffer_usage         = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    //         .require_device_local = TRUE,
    //         .require_host_visible = TRUE,
    //         .size                 = sizeof(f32),
    //     }
    //     );

    // vc_image img = vc_image_allocate(
    //     &ctx,
    //     (image_create_desc){
    //         .image_dimension = 2,
    //         .image_format    = VK_FORMAT_R8G8B8A8_UINT,
    //         .width           = 1916,
    //         .height          = 1036,
    //         .depth           = 1,
    //         .image_usage     = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    //         .share           = FALSE,
    //         .layout          = VK_IMAGE_LAYOUT_GENERAL,
    //     }
    //     );

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

    descriptor_set_desc descriptor_desc =
    {
        .binding_count = 1,
        .bindings      = (descriptor_binding_desc[1]){
            [0] = (descriptor_binding_desc){
                .descriptor_type  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptor_count = 1,
                .stage_flags      = VK_SHADER_STAGE_COMPUTE_BIT,
                .image_info       = &(descriptor_binding_image){ 0 },
            }
        }
    };

    vc_descriptor_set_layout set_layout = vc_descriptor_set_layout_create(&ctx, descriptor_desc);
    vc_descriptor_set sets[vc_swapchain_image_count(&ctx)];

    for (int i = 0; i < vc_swapchain_image_count(&ctx); i++)
    {
        descriptor_desc.bindings[0].image_info = &(descriptor_binding_image)
        {
            .layout = VK_IMAGE_LAYOUT_GENERAL,
            .image  = vc_swapchain_get_image_hndls(&ctx)[i],
        };
        sets[i] = vc_descriptor_set_create(&ctx, descriptor_desc, set_layout);
    }
    u64 source_size      = 0;
    u8 *source           = fio_read_whole_file("shaders/test.comp.spv", &source_size);
    vc_compute_pipe pipe = vc_compute_pipe_create(
        &ctx,
        &(compute_pipe_desc){
            .shader_code_length = source_size,
            .shader_code        = source,
            .set_layout         = set_layout,
        }
        );

    vc_command_buffer buf = vc_command_buffer_main_create(&ctx, VC_QUEUE_COMPUTE);

    vc_semaphore sem = vc_semaphore_create(&ctx);
    (void)sem;

    (void)buf;
    (void)pipe;
    while ( !glfwWindowShouldClose(window) )
    {
        vc_queue_wait_idle(&ctx, VC_QUEUE_COMPUTE);
        vc_queue_wait_idle(&ctx, VC_QUEUE_MAIN);
        u32 iid = 0;
        vc_swapchain_acquire_image(&ctx, &iid, sem);

        vc_command_buffer_reset(&ctx, buf);
        vc_command_buffer_begin(&ctx, buf);

        vc_image curi = vc_swapchain_get_image_hndls(&ctx)[iid];
        vc_command_image_pipe_barrier(
            &ctx,
            buf,
            curi,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_ACCESS_MEMORY_READ_BIT,
            VK_ACCESS_MEMORY_WRITE_BIT,
            VC_QUEUE_IGNORED,
            VC_QUEUE_IGNORED
            );

        vc_command_buffer_bind_descriptor_set(&ctx, buf, pipe, sets[iid]);
        vc_command_buffer_compute_pipeline(
            &ctx,
            buf,
            &(compute_dispatch_desc){
                .pipe     = pipe,
                .groups_x = 120,
                .groups_y = 65,
                .groups_z = 1,
            }
            );

        vc_command_image_pipe_barrier(
            &ctx,
            buf,
            curi,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_ACCESS_MEMORY_WRITE_BIT,
            VK_ACCESS_MEMORY_READ_BIT,
            VC_QUEUE_IGNORED,
            VC_QUEUE_IGNORED
            );

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