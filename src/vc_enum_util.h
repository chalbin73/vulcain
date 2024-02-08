#include "vulcain.h"
#include <vulkan/vulkan.h>

/* ---------------- Enum helpers ---------------- */
const char *vc_priv_VkColorSpaceKHR_to_str(VkColorSpaceKHR    input_value);
const char *vc_priv_VkFormat_to_str(VkFormat    input_value);
const char *vc_priv_VkResult_to_str(VkResult    input_value);
//VkPipelineBindPoint    vc_priv_pipeline_type_to_bind_point(vc_pipeline_type    type);
char       *vc_priv_VkDebugUtilsMessageSeverityFlagBitsEXT_to_str(VkDebugUtilsMessageSeverityFlagBitsEXT    input_value);
char       *vc_priv_VkDebugUtilsMessageSeverityFlagBitsEXT_to_prefix_str(VkDebugUtilsMessageSeverityFlagBitsEXT    input_value);

/* ---------------- Error checking ---------------- */
#define VK_CHECK(s, m)                                                                                                 \
        do                                                                                                                 \
        {                                                                                                                  \
            VkResult _res = s;                                                                                             \
            if (_res != VK_SUCCESS)                                                                                        \
            {                                                                                                              \
                vc_error( "VKERROR: '%s' %s:%d error=%d:(%s).", m, __FILE__, __LINE__, _res, vc_priv_VkResult_to_str(_res) ); \
            }                                                                                                              \
        }                                                                                                                  \
        while (0);
#define VK_CHECKR(s, m)                                                                                                \
        do                                                                                                                 \
        {                                                                                                                  \
            VkResult _res = s;                                                                                             \
            if (_res != VK_SUCCESS)                                                                                        \
            {                                                                                                              \
                vc_error( "VKERROR: '%s' %s:%d error=%d:(%s).", m, __FILE__, __LINE__, _res, vc_priv_VkResult_to_str(_res) ); \
                return FALSE;                                                                                              \
            }                                                                                                              \
        }                                                                                                                  \
        while (0);

#define VK_CHECKH(s, m)                                                                                                \
        do                                                                                                                 \
        {                                                                                                                  \
            VkResult _res = s;                                                                                             \
            if (_res != VK_SUCCESS)                                                                                        \
            {                                                                                                              \
                vc_error( "VKERROR: '%s' %s:%d error=%d:(%s).", m, __FILE__, __LINE__, _res, vc_priv_VkResult_to_str(_res) ); \
                return VC_NULL_HANDLE;                                                                                              \
            }                                                                                                              \
        }                                                                                                                  \
        while (0);

