#include <vulcain/femtolog.h>
#include <vulcain/vulcain.h>
#include <vulcain/vc_device.h>

#define VC_WS_GLFW
#include <vulcain/win_sys/vc_glfw.h>
#include <vulcain/utils/vc_imgui.h>

vc_descriptor_set_layout pipe_layout = VC_NULL_HANDLE;

//vc_descriptor_set *image_sets;
//vc_image_view *image_views;
vc_semaphore sig_sem;

i32 size[2];
i32 size_2[2];
//VkFormat swapchain_format;

uint64_t
device_score(void *ud, VkPhysicalDevice phy)
{
    return 1;
}

GLFWwindow *window;
GLFWwindow *window_2;

void
create_cbk(vc_ctx *ctx, void *udata, vc_swapchain_created_info info)
{
    return;
    /*
       swapchain_format = info.swapchain_image_format;
       image_views      = mem_allocate(sizeof(vc_image_view) * info.swapchain_image_count, MEMORY_TAG_RENDER_DATA);
       image_sets       = mem_allocate(sizeof(vc_descriptor_set) * info.swapchain_image_count, MEMORY_TAG_RENDER_DATA);

       for(u32 i = 0; i < info.swapchain_image_count; i++)
       {
        image_sets[i] = vc_descriptor_set_allocate(ctx, pipe_layout);

        vc_descriptor_set_writer writer =
        {
            0
        };

        image_views[i] = vc_image_view_create(ctx, info.images[i], VK_IMAGE_VIEW_TYPE_2D, VC_COMP_MAP_ID, VC_IMG_SUBRES_COLOR_1);
        vc_descriptor_set_writer_write_image(ctx, &writer, 0, 0, image_views[i], VC_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        vc_descriptor_set_writer_write(ctx, &writer, image_sets[i]);
       }
     */
}

void
destr_cbk(vc_ctx *ctx, void *udata, vc_swapchain_created_info info)
{
    return;
    /*
       for(u32 i = 0; i < info.swapchain_image_count; i++)
       {
        vc_handle_destroy(ctx, image_views[i]);
        vc_handle_destroy(ctx, image_sets[i]);
       }

       mem_free(image_views);
       mem_free(image_sets);
     */
}

int
main(int argc, char **argv)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window   = glfwCreateWindow(1920, 1080, "Vulcain", NULL, NULL);
    window_2 = glfwCreateWindow(1920, 1080, "Vulcain_2", NULL, NULL);

    if(!window_2)
    {
        printf("Failed to create second window.\n");
    }

    glfwShowWindow(window_2);
    glfwShowWindow(window);

    vc_ctx ctx =
    {
        0
    };
    u32 cnt           = 0;
    const char **exts = glfwGetRequiredInstanceExtensions(&cnt);

    vc_ctx_create(
        &ctx,
        (VkApplicationInfo)
        {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .apiVersion         = VK_MAKE_VERSION(1, 3, 0),
            .pEngineName        = "Waltuh",
            .pApplicationName   = "Waltuh",
            .engineVersion      = VK_MAKE_VERSION(4, 2, 0),
            .applicationVersion = VK_MAKE_VERSION(6, 9, 0)
        },
        NULL,
        TRUE,
        0,
        NULL,
        cnt,
        exts
        );

    vc_device_builder b           = vc_device_builder_begin(&ctx);
    vc_windowing_system glfw_ws   = vc_ws_glfw(window);
    vc_windowing_system glfw_ws_2 = vc_ws_glfw(window_2);


    vc_device_builder_set_score_func(b, device_score, NULL);

    vc_queue comp_queue = vc_device_builder_add_queue(b, VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT);
    (void)main;
    vc_queue pres_queue;
    vc_device_builder_request_presentation_support( b, &pres_queue, glfw_ws );
    vc_device_builder_request_extension(b, "VK_KHR_swapchain");

    vc_device_builder_end(b);
    (void)comp_queue;

    {
        vc_descriptor_set_layout_builder builder =
        {
            0
        };
        vc_descriptor_set_layout_builder_add_binding(&builder, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
        pipe_layout = vc_descriptor_set_layout_builder_build(&ctx, &builder, 0);
    }


    u64 code_size                 = 0;
    u8 *code                      = fio_read_whole_file("pg_shaders/test.comp.spv", &code_size);
    vc_compute_pipeline comp_pipe = vc_compute_pipeline_create(
        &ctx,
        code,
        code_size,
        "main",
        (vc_pipeline_layout_info)
        {
            .set_layout_count     = 1,
            .set_layouts          = &pipe_layout,
            .push_constants_count = 1,
            .push_constants       = &(VkPushConstantRange)
            {
                .size       = sizeof(size),
                .offset     = 0,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            }
        }
        );
    (void)comp_pipe;

    vc_swapchain swapchain = vc_swapchain_create(
        &ctx,
        glfw_ws,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        (vc_format_query)
        {
            .required_optimal_tiling_features = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT,
        },
        create_cbk,
        destr_cbk,
        NULL
        );

    vc_swapchain swapchain_2 = vc_swapchain_create(
        &ctx,
        glfw_ws_2,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        (vc_format_query)
        {
            .required_optimal_tiling_features = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT,
        },
        create_cbk,
        destr_cbk,
        NULL
        );

    vc_swapchain_created_info swp_i;
    vc_swapchain_get_info(&ctx, swapchain, &swp_i);
    vc_imgui_setup(&ctx, comp_queue, glfw_ws, swp_i.swapchain_image_format);

    u64 vert_size, frag_size;
    u8 *vert_code, *frag_code;

    vert_code = fio_read_whole_file("pg_shaders/tri.vert.spv", &vert_size);
    frag_code = fio_read_whole_file("pg_shaders/tri.frag.spv", &frag_size);


    vc_gfx_pipeline gfx_pipe = vc_gfx_pipeline_dynamic_create(
        &ctx,
        (vc_graphics_pipeline_desc)
        {
            .layout_info.push_constants_count = 0,
            .layout_info.set_layout_count     = 0,

            .shader_code = (vc_gfx_pipeline_code_info)
            {
                .vertex_code          = vert_code,
                .fragment_code        = frag_code,
                .vertex_code_size     = vert_size,
                .fragment_code_size   = frag_size,
                .vertex_entry_point   = "main",
                .fragment_entry_point = "main",
            },

            .vertex_binding_count   = 0,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .line_width             = 1.0f,
            .viewport_scissor_count = 1,
            .viewports              = &(VkViewport)
            {
                .x        = 0,
                .y        = 0,
                .width    = 1366,
                .height   = 768,
                .maxDepth = 1.0f,
                .minDepth = 0.0f,
            },
            .scissors = &(VkRect2D)
            {
                .offset = { 0 },
                .extent = { 1366, 768 },
            },

            .depth_test = FALSE,

            .stencil_test = FALSE,

            .enable_depth_clamp = FALSE,

            .enable_depth_bias = FALSE,

            .sample_count = VK_SAMPLE_COUNT_1_BIT,

            .attachment_count  = 1,
            .attachment_blends =
                &(VkPipelineColorBlendAttachmentState){
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .colorBlendOp        = VK_BLEND_OP_ADD,
                .alphaBlendOp        = VK_BLEND_OP_ADD,
                .blendEnable         = VK_FALSE,
                .colorWriteMask      = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,
            },
            .blend_constants     = { 1.0f, 1.0f, 1.0f, 1.0f },
            .dynamic_state_count = 0,
        },
        (vc_pipeline_rendering_info)
        {
            .view_mask                = 0,
            .color_attachment_count   = 1,
            .color_attachment_formats = &swp_i.swapchain_image_format,
        }
        );

    vc_command_buffer comp_buf = vc_command_buffer_allocate(
        &ctx,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        vc_command_pool_create(&ctx, comp_queue, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
        );

    sig_sem = vc_semaphore_create(&ctx);


    float clear_color[3];
    u64 prev_time = platform_millis();
    while( !glfwWindowShouldClose(window) && !glfwWindowShouldClose(window_2) )
    {
        u64 now_time = platform_millis();
        if(now_time - prev_time < 16)
        {
            continue;
        }
        prev_time = now_time;

        glfwGetFramebufferSize(window, &size[0], &size[1]);
        glfwGetFramebufferSize(window_2, &size_2[0], &size_2[1]);

        vc_semaphore sem;
        vc_semaphore sem_2;
        vc_device_wait_idle(&ctx);
        vc_swpchn_img_id id   = vc_swapchain_acquire_image(&ctx, swapchain, &sem);
        vc_swpchn_img_id id_2 = vc_swapchain_acquire_image(&ctx, swapchain_2, &sem_2);

        vc_swapchain_created_info created_i;
        vc_swapchain_get_info(&ctx, swapchain, &created_i);
        vc_swapchain_created_info created_i_2;
        vc_swapchain_get_info(&ctx, swapchain_2, &created_i_2);


        //puts("Beginning");
        vc_cmd_record rec = vc_command_buffer_begin(&ctx, comp_buf, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        vc_cmd_image_barrier(
            rec,
            created_i.images[id],
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_NONE,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            //VK_IMAGE_LAYOUT_GENERAL,
            (VkImageSubresourceRange)
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount     = 1,
                .levelCount     = 1,
                .baseMipLevel   = 0,
                .baseArrayLayer = 0
            },
            VC_NULL_HANDLE,
            VC_NULL_HANDLE
            );

        vc_cmd_image_barrier(
            rec,
            created_i_2.images[id_2],
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_NONE,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            //VK_IMAGE_LAYOUT_GENERAL,
            (VkImageSubresourceRange)
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount     = 1,
                .levelCount     = 1,
                .baseMipLevel   = 0,
                .baseArrayLayer = 0
            },
            VC_NULL_HANDLE,
            VC_NULL_HANDLE
            );

        vc_rendering_info render_info = (vc_rendering_info)
        {
            .view_mask          = 0,
            .stencil_attachment = VC_NULL_HANDLE,
            .depth_attachment   = VC_NULL_HANDLE,
            .color_attachments  = &(vc_rendering_attachment_info)
            {
                .clear_value = (VkClearValue)
                {
                    .color = (VkClearColorValue)
                    {
                        .float32 =
                        {
                            clear_color[0], clear_color[1], clear_color[2], 0.1f
                        }
                    }
                },
                .load_op            = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .store_op           = VK_ATTACHMENT_STORE_OP_STORE,
                .image_view         = created_i.image_views[id],
                .resolve_image_view = VC_NULL_HANDLE,
                .image_layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
            .color_attachments_count = 1,
            .layer_count             = 1,
            .render_area             = (VkRect2D)
            {
                .offset =
                {
                    0, 0
                }, .extent = created_i.swapchain_extent
            },
        };

        vc_cmd_begin_rendering(
            rec,
            render_info
            );

        vc_cmd_bind_pipeline(rec, gfx_pipe);
        vc_cmd_draw(rec, 3, 1, 0, 0);

        vc_cmd_end_rendering(rec);

        render_info.render_area = (VkRect2D)
        {
            .offset =
            {
                0
            }, .extent = created_i_2.swapchain_extent
        };

        render_info.color_attachments->image_view = created_i_2.image_views[id_2];

        vc_cmd_begin_rendering(
            rec,
            render_info
            );

        vc_cmd_bind_pipeline(rec, gfx_pipe);
        vc_cmd_draw(rec, 3, 1, 0, 0);

        vc_cmd_end_rendering(rec);

        /*
           vc_cmd_bind_descriptor_set(rec, comp_pipe, image_sets[id], 0);
           vc_cmd_push_constants(rec, comp_pipe, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(size), size);
           vc_cmd_dispatch_compute(rec, comp_pipe, 1920 / 16, 1080 / 16, 1);
         */

        vc_imgui_begin_frame(&ctx);

        igBegin("Window", NULL, 0);

        static bool b = FALSE;
        igText("Hello ! ImGui's working !");
        igText("This is some useful text");
        igCheckbox("Demo window", &b);
        igCheckbox("Another window", &b);

        static f32 f = 0.0f;
        igSliderFloat("Float", &f, 0.0f, 1.0f, "%.3f", 0);
        igColorEdit3("clear color", (float *)&clear_color, 0);

        igEnd();

        vc_cmd_imgui_end_frame_render(
            rec,
            created_i.image_views[id],
            (VkRect2D){ .offset = { 0 }, .extent = { size[0], size[1] } },
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            );

        vc_cmd_image_barrier(
            rec,
            created_i_2.images[id_2],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_NONE,
            //VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            (VkImageSubresourceRange)
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount     = 1,
                .levelCount     = 1,
                .baseMipLevel   = 0,
                .baseArrayLayer = 0
            },
            VC_NULL_HANDLE,
            VC_NULL_HANDLE
            );

        vc_cmd_image_barrier(
            rec,
            created_i.images[id],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_NONE,
            //VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            (VkImageSubresourceRange)
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount     = 1,
                .levelCount     = 1,
                .baseMipLevel   = 0,
                .baseArrayLayer = 0
            },
            VC_NULL_HANDLE,
            VC_NULL_HANDLE
            );

        vc_command_buffer_end(rec);
        vc_command_buffer_submit(&ctx, comp_buf, comp_queue, 2, (vc_semaphore[2]) { sem, sem_2 }, (VkPipelineStageFlags[2]){ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT }, 1, &sig_sem);

        vc_swapchain_present_images(&ctx, 2, (vc_swapchain[2]){ swapchain, swapchain_2 }, (vc_swpchn_img_id[2]){ id, id_2 }, pres_queue, 1, &sig_sem );

        glfwPollEvents();
    }
    printf("End !!\n");
    vc_ctx_destroy(&ctx);

    glfwDestroyWindow(window);
    glfwDestroyWindow(window_2);
    glfwTerminate();

    return 0;
}

