#include "vc_windowing_systems.h"

#if VC_ENABLE_WINDOWING_GLFW
void _vc_priv_windowing_glfw_frambuffer_size(GLFWwindow *window, u32 *width, u32 *height)
{
    i32 iwidth, iheight = 0;
    glfwGetFramebufferSize(window, &iwidth, &iheight);

    *width = iwidth;
    *height = iheight;
}

VkResult _vc_priv_windowing_glfw_get_window_surface(GLFWwindow *window, VkInstance inst, VkSurfaceKHR *out_surface)
{
    return glfwCreateWindowSurface(inst, window, NULL, out_surface);
}

vc_windowing_system_funcs vc_windowing_system_glfw(GLFWwindow *window)
{
    vc_windowing_system_funcs funcs =
    {
        .windowing_ctx = window,
        .get_framebuffer_size_fun = (vc_get_framebuffer_size_fun)_vc_priv_windowing_glfw_frambuffer_size,
        .get_window_surface_fun = (vc_get_window_surface_fun)_vc_priv_windowing_glfw_get_window_surface,
    };
    return funcs;
}

#endif