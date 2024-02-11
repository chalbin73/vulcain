#include <vulcain/femtolog.h>
#include <vulcain/vulcain.h>
#include <vulcain/vc_device.h>

#define VC_WS_GLFW
#include <vulcain/win_sys/vc_glfw.h>

vc_descriptor_set_layout pipe_layout = VC_NULL_HANDLE;

vc_descriptor_set *image_sets;
vc_image_view *image_views;

uint64_t
device_score(void *ud, VkPhysicalDevice phy)
{
    return 1;
}

GLFWwindow *window;

void
create_cbk(vc_ctx *ctx, void *udata, vc_swapchain_created_info info)
{
    return;
    image_views = mem_allocate(sizeof(vc_image_view) * info.swapchain_image_count, MEMORY_TAG_RENDER_DATA);
    image_sets  = mem_allocate(sizeof(vc_descriptor_set) * info.swapchain_image_count, MEMORY_TAG_RENDER_DATA);

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
}

void
destr_cbk(vc_ctx *ctx, void *udata, vc_swapchain_created_info info)
{
    return;
    for(u32 i = 0; i < info.swapchain_image_count; i++)
    {
        vc_handle_destroy(ctx, image_views[i]);
        vc_handle_destroy(ctx, image_sets[i]);
    }

    mem_free(image_views);
    mem_free(image_sets);
}

int
main(int argc, char **argv)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(1920, 1080, "Hello", NULL, NULL);
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

    vc_device_builder b         = vc_device_builder_begin(&ctx);
    vc_windowing_system glfw_ws = vc_ws_glfw(window);


    vc_device_builder_set_score_func(b, device_score, NULL);

    vc_queue comp_queue = vc_device_builder_add_queue(b, VK_QUEUE_COMPUTE_BIT);
    (void)main;
    vc_queue pres_queue;
    vc_device_builder_request_presentation_support( b, &pres_queue, glfw_ws );
    vc_device_builder_request_extension(b, "VK_KHR_swapchain");

    vc_device_builder_end(b);
    (void)comp_queue;

    /*
       {
        vc_descriptor_set_layout_builder builder =
        {
            0
        };
        vc_descriptor_set_layout_builder_add_binding(&builder, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
        pipe_layout = vc_descriptor_set_layout_builder_build(&ctx, &builder, 0);
       }

       u64 code_size            = 0;
       u8 *code                 = fio_read_whole_file("pg_shaders/test.comp.spv", &code_size);
       vc_compute_pipeline pipe = vc_compute_pipeline_create(&ctx, code, code_size, "main", 1, &pipe_layout, 0, NULL);
       (void)pipe;
     */

    vc_swapchain swapchain = vc_swapchain_create(
        &ctx,
        glfw_ws,
        VK_IMAGE_USAGE_STORAGE_BIT,
        (vc_format_query)
        {
            .required_optimal_tiling_features = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT,
        },
        create_cbk,
        destr_cbk,
        NULL
        );

    vc_command_buffer comp_buf = vc_command_buffer_allocate(
        &ctx,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        vc_command_pool_create(&ctx, comp_queue, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
        );



    vc_semaphore sem     = vc_semaphore_create(&ctx);
    vc_semaphore sig_sem = vc_semaphore_create(&ctx);

    u64 prev_time = platform_millis();
    while( !glfwWindowShouldClose(window) )
    {
        u64 now_time = platform_millis();
        if(now_time - prev_time < 100)
        {
            continue;
        }
        prev_time = now_time;

        vc_device_wait_idle(&ctx);
        vc_swpchn_img_id id = vc_swapchain_acquire_image(&ctx, swapchain, sem);

        //puts("Beginning");
        vc_cmd_record rec = vc_command_buffer_begin(&ctx, comp_buf, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        vc_cmd_image_barrier(
            rec,
            vc_swapchain_get_image(&ctx, swapchain, id),
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_ACCESS_NONE,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL,
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

        //vc_cmd_bind_descriptor_set(rec, pipe, image_sets[id], 0);
        //vc_cmd_dispatch_compute(rec, pipe, 1920 / 16, 1080 / 16, 1);

        vc_cmd_image_barrier(
            rec,
            vc_swapchain_get_image(&ctx, swapchain, id),
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_ACCESS_NONE,
            VK_IMAGE_LAYOUT_GENERAL,
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
        vc_command_buffer_submit(&ctx, comp_buf, comp_queue, 1, &sem, (VkPipelineStageFlags[1]){ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT }, 1, &sig_sem);

        vc_swapchain_present_image(&ctx, swapchain, pres_queue, sig_sem, id);

        glfwPollEvents();
        //       break;
    }
    printf("End !!\n");
    vc_ctx_destroy(&ctx);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

