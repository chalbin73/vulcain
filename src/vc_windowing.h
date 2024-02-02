#ifndef __VC_WINDOWING_H__
#define __VC_WINDOWING_H__

#include <vulkan/vulkan.h>

typedef VkResult (*vc_ws_create_surface_func)(VkInstance instance, void *ws_udata, const VkAllocationCallbacks *allocator, VkSurfaceKHR *surface);
typedef void (*vc_ws_get_fb_size_func)(void *udata, uint32_t *width, uint32_t *height);

// Will contain everything needed to communicate with a windowing system in order to create surface/swapchain.
typedef struct
{
    void                        *udata;
    char                         windowing_system_name[256];

    vc_ws_create_surface_func    create_surface;
    vc_ws_get_fb_size_func get_fb_size;
} vc_windowing_system;

#endif //__VC_WINDOWING_H__

