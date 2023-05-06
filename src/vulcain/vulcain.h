#include "../base/base.h"
#include <vulkan/vulkan.h>

typedef struct
{
    char  *engine_name;
    char  *app_name;
    u32    app_version;
    u32    engine_version;
    b8     enable_debugging;
    u32    extension_count;
    char **extensions;
} instance_desc;

typedef struct
{
    VkInstance vk_instance;
} vc_ctx;

#define VK_CHECK(s, m) do{ VkResult _res = s; if(_res != VK_SUCCESS) {ERROR("VKERROR: '%s' %s:%d error=%d.", m, __FILE__, __LINE__, _res);} }while(0);
#define VK_CHECKR(s, m) do{ VkResult _res = s; if(_res != VK_SUCCESS) {ERROR("VKERROR: '%s' %s:%d error=%d.", m, __FILE__, __LINE__, _res); return FALSE; } }while(0);

b8 vc_create_ctx(vc_ctx *ctx, instance_desc *desc);