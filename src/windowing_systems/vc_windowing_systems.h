#pragma once

#include "../base.h"
#include <vulkan/vulkan.h>

typedef void (*vc_get_framebuffer_size_fun)(void *user_data, u32 *width, u32 *height);
typedef VkResult (*vc_get_window_surface_fun)(void *user_data, VkInstance inst, VkSurfaceKHR *out_surface);

typedef struct
{
    void                          *windowing_ctx;
    vc_get_framebuffer_size_fun    get_framebuffer_size_fun;
    vc_get_window_surface_fun      get_window_surface_fun;
} vc_windowing_system_funcs;
#define VC_ENABLE_WINDOWING_GLFW 1
#if VC_ENABLE_WINDOWING_GLFW
#include <GLFW/glfw3.h>
vc_windowing_system_funcs    vc_windowing_system_glfw(GLFWwindow   *window);
#endif
