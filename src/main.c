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

    u64             source_size = 0;
    u8             *source = fio_read_whole_file("shaders/test.comp.spv", &source_size);
    vc_compute_pipe pipe = vc_compute_pipe_create(&ctx, &(compute_pipe_desc){
                                                            .shader_code_length = source_size,
                                                            .shader_code = source,
                                                            .binding_count = 0,
                                                            .bindings = NULL,
                                                        });

    (void)pipe;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    vc_destroy_ctx(&ctx);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}