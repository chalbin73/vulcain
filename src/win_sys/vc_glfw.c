#include "../vc_windowing.h"
#include "vc_glfw.h"
#include <string.h>

VkResult
_vc_ws_create_surface_glfw(VkInstance instance, void *ws_udata, const VkAllocationCallbacks *allocator, VkSurfaceKHR *surface)
{
    GLFWwindow *win = (GLFWwindow *)ws_udata;

    return glfwCreateWindowSurface(instance, win, allocator, surface);
}

vc_windowing_system
vc_ws_glfw(GLFWwindow   *window)
{
    vc_windowing_system sys;
    strcpy(sys.windowing_system_name, "GLFW3");

    sys.create_surface = _vc_ws_create_surface_glfw;
    sys.udata = window;

    return sys;
}

