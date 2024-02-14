#include "../vc_windowing.h"
#include "vc_glfw.h"
#include <string.h>
#include "../base/types.h"

VkResult
_vc_ws_create_surface_glfw(VkInstance instance, void *ws_udata, const VkAllocationCallbacks *allocator, VkSurfaceKHR *surface)
{
    GLFWwindow *win = (GLFWwindow *)ws_udata;

    return glfwCreateWindowSurface(instance, win, allocator, surface);
}

void
_vc_ws_get_fb_size_glfw(void *udata, u32 *w, u32 *h)
{
    GLFWwindow *win = (GLFWwindow *)udata;

    i32 w_r = 0;
    i32 h_r = 0;
    glfwGetFramebufferSize(win, &w_r, &h_r);

    *w = (u32)w_r;
    *h = (u32)h_r;
}

#define CIMGUI_USE_GLFW
#include "../utils/vc_imgui.h"

void
_vc_ws_ig_init(void   *udata)
{
    GLFWwindow *win = (GLFWwindow *)udata;

    ImGui_ImplGlfw_InitForVulkan(win, TRUE);
}

void
_vc_ws_ig_begin_frame(void   *udata)
{
    //GLFWwindow *win = (GLFWwindow *)udata;
    ImGui_ImplGlfw_NewFrame();
}

void
_vc_ws_ig_shutdown(void   *udata)
{
    //GLFWwindow *win = (GLFWwindow *)udata;
    ImGui_ImplGlfw_Shutdown();
}

vc_windowing_system
vc_ws_glfw(GLFWwindow   *window)
{
    vc_windowing_system sys;
    strcpy(sys.windowing_system_name, "GLFW3");

    sys.create_surface = _vc_ws_create_surface_glfw;
    sys.get_fb_size    = _vc_ws_get_fb_size_glfw;
    sys.ig_init        = _vc_ws_ig_init;
    sys.ig_begin_frame = _vc_ws_ig_begin_frame;
    sys.ig_shutdown    = _vc_ws_ig_shutdown;
    sys.udata          = window;


    return sys;
}

