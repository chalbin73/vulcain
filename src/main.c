#include "base/fio.h"
#include "vulcain/vc_handles.h"
#include <vulkan/vulkan_core.h>
#define VC_ENABLE_WINDOWING_GLFW 1

#include "base/base.h"
#include "vulcain/vulcain.h"
#include <GLFW/glfw3.h>

GLFWwindow *window;

int main(i32 argc, char **argv)
{
    INFO("Hello, World ! Welcome to vulcain !");

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1920, 1080, "Hello !", NULL, NULL);
    glfwShowWindow(window);
    u32          exts_count = 0;
    const char **exts = glfwGetRequiredInstanceExtensions(&exts_count);

    vc_ctx ctx = {0};
    vc_create_ctx(&ctx, &(instance_desc){
                            .app_name = "Playground",
                            .engine_name = "Playground",
                            .engine_version = VK_MAKE_VERSION(0, 0, 0),
                            .enable_debugging = TRUE,
                            .extension_count = exts_count,
                            .extensions = (char **)exts,
                            .enable_windowing = TRUE,
                            .windowing_system = vc_windowing_system_glfw(window),
                        },
                  &(physical_device_query){.allowed_types = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU | VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, .requested_features = {.geometryShader = TRUE}, .request_main_queue = TRUE, .request_compute_queue = TRUE, .request_transfer_queue = FALSE});

    descriptor_set_desc descriptor_desc = (descriptor_set_desc){
        .binding_count = 1,
        .bindings = (descriptor_binding_desc[1]){
            [0] = (descriptor_binding_desc){
                .descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptor_count = 1,
                .stage_flags = VK_SHADER_STAGE_COMPUTE_BIT,
            }}};

    vc_descriptor_set_layout set_layout = vc_descriptor_set_layout_create(&ctx, descriptor_desc);

    u64             source_size = 0;
    u8             *source = fio_read_whole_file("shaders/test.comp.spv", &source_size);
    vc_compute_pipe pipe = vc_compute_pipe_create(&ctx, &(compute_pipe_desc){
                                                            .shader_code_length = source_size,
                                                            .shader_code = source,
                                                            .set_layout = set_layout,
                                                        });

    vc_command_buffer buf = vc_command_buffer_main_create(&ctx, VC_QUEUE_COMPUTE);

    vc_semaphore sem = vc_semaphore_create(&ctx);
    (void)sem;

    (void)buf;
    (void)pipe;
    while (!glfwWindowShouldClose(window))
    {
        vc_queue_wait_idle(&ctx, VC_QUEUE_COMPUTE);
        vc_queue_wait_idle(&ctx, VC_QUEUE_MAIN);
        u32 iid = 0;
        vc_swapchain_acquire_image(&ctx, &iid, sem);

        vc_command_buffer_reset(&ctx, buf);
        vc_command_buffer_begin(&ctx, buf);

        vc_image curi = vc_swapchain_get_image_hndls(&ctx)[iid];
        vc_cmd_image_pipe_barrier(&ctx, buf, curi, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_GENERAL,
                                  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                  VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_MEMORY_WRITE_BIT,
                                  VC_QUEUE_IGNORED, VC_QUEUE_IGNORED);

        vc_command_buffer_compute_pipeline(&ctx, buf, &(compute_dispatch_desc){
                                                          .pipe = pipe,
                                                          .groups_x = 1,
                                                          .groups_y = 1,
                                                          .groups_z = 1,
                                                      });

        vc_cmd_image_pipe_barrier(&ctx, buf, curi, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                  VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
                                  VC_QUEUE_IGNORED, VC_QUEUE_IGNORED);

        vc_command_buffer_end(&ctx, buf);
        vc_command_buffer_submit(&ctx, buf, sem, (VkPipelineStageFlags[1]){[0] = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT});
        vc_queue_wait_idle(&ctx, VC_QUEUE_COMPUTE);

        vc_swapchain_present_image(&ctx, iid);

        glfwPollEvents();
    }

    vc_destroy_ctx(&ctx);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
