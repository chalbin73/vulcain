#ifndef __VC_WINDOWING_H__
#define __VC_WINDOWING_H__

#include <vulkan/vulkan.h>

typedef VkResult (*vc_ws_create_surface_func)(VkInstance instance, void *ws_udata, const VkAllocationCallbacks *allocator, VkSurfaceKHR *surface);
typedef void (*vc_ws_get_fb_size_func)(void *udata, uint32_t *width, uint32_t *height);

// Imgui functions
typedef void (*vc_ws_ig_init)(void   *udata);
typedef void (*vc_ws_ig_begin_frame)(void   *udata);
typedef void (*vc_ws_ig_shutdown)(void   *udata);

// Will contain everything needed to communicate with a windowing system in order to create surface/swapchain.
typedef struct
{
    void                        *udata;
    char                         windowing_system_name[256];

    vc_ws_create_surface_func    create_surface;
    vc_ws_get_fb_size_func       get_fb_size;

    // IMGUI. Does not need to be set if not using imgui
    vc_ws_ig_init                ig_init;
    vc_ws_ig_begin_frame ig_begin_frame;
    vc_ws_ig_shutdown ig_shutdown;
} vc_windowing_system;

#endif //__VC_WINDOWING_H__

