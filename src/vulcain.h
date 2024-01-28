#ifndef __VULCAIN_H__
#define __VULCAIN_H__

#include "vc_windowing.h"
#include "handles/vc_handles.h"
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include <stdint.h>

#include "femtolog.h"

#define vc_trace(fmt, ...) \
        fl_log(TRACE, __FILE__, __LINE__, fmt, ## __VA_ARGS__);

#define vc_debug(fmt, ...) \
        fl_log(DEBUG, __FILE__, __LINE__, fmt, ## __VA_ARGS__);

#define vc_info(fmt, ...) \
        fl_log(INFO, __FILE__, __LINE__, fmt, ## __VA_ARGS__);

#define vc_warn(fmt, ...) \
        fl_log(WARN, __FILE__, __LINE__, fmt, ## __VA_ARGS__);

#define vc_error(fmt, ...) \
        fl_log(ERROR, __FILE__, __LINE__, fmt, ## __VA_ARGS__);

#define vc_fatal(fmt, ...) \
        fl_log(FATAL, __FILE__, __LINE__, fmt, ## __VA_ARGS__);


// Welcome to vulcain
typedef struct
{
    vc_handles_manager          handles_manager;

    bool                        windowing_enabled; // This means that the application runs in some sort of a window, so swapchains can be created.
    bool                        debugging_enabled; // This is true if some validation layers are requested.

    vc_windowing_system         windowing_system; // In the case a windowing system is being used.

    VkInstance                  vk_instance;
    VkDebugUtilsMessengerEXT    debugging_messenger; // Only used if debugging_enabled.

    VkDevice                    current_device;
} vc_ctx;

typedef struct
{
    uint32_t    format_count;
    VkFormat   *formats;
} vc_format_set;

typedef struct
{
    VkFormatFeatureFlags    required_linear_tiling_features;
    VkFormatFeatureFlags    required_optimal_tiling_features;
    VkFormatFeatureFlags    required_buffer_features;
} vc_format_query;

bool vc_ctx_create(vc_ctx                *ctx,
                   VkApplicationInfo      app_info,
                   vc_windowing_system   *windowing_system,
                   bool                   enable_debugging,
                   uint32_t               layer_count,
                   const char           **layer_names,
                   uint32_t               extension_count,
                   const char           **extension_names);

void vc_ctx_destroy(vc_ctx   *ctx);

#endif //__VULCAIN_H__

