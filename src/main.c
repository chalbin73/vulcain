#include "base/base.h"
#include "vulcain/vulcain.h"
#include <GLFW/glfw3.h>

GLFWwindow *window;

void glfw_frambuffer_size(GLFWwindow *window, u32 *width, u32 *height)
{
    i32 iwidth, iheight = 0;
    glfwGetFramebufferSize(window, &iwidth, &iheight);

    *width = iwidth;
    *height = iheight;
}

int main(i32 argc, char **argv)
{
    INFO("Hello, World ! Welcome to vulcain !");

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1920, 1080, "Hello !", NULL, NULL);
    glfwShowWindow(window);

    u32 exts_count = 0;
    const char **exts = glfwGetRequiredInstanceExtensions(&exts_count);

    vc_ctx ctx = {0};
    vc_create_ctx(&ctx, &(instance_desc){
        .app_name = "Playground",
        .engine_name = "Playground",
        .engine_version = VK_MAKE_VERSION(0, 0, 0),
        .enable_debugging = TRUE,
        .extension_count = exts_count,
        .extensions = (char **)exts
    });
    vc_get_surface_glfw(&ctx, window);

    vc_select_create_device(&ctx, (physical_device_query)
    {
        .allowed_types = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU | VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
        .requested_features =
        {
            .geometryShader = TRUE
        },
        .request_main_queue = TRUE,
        .request_compute_queue = TRUE,
        .request_transfer_queue = FALSE
    });
    
    vc_setup_default_swapchain(&ctx, (vc_get_framebuffer_size_fun)glfw_frambuffer_size, window);

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    vc_destroy_ctx(&ctx);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}