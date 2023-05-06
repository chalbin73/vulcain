#include "base/base.h"
#include "vulcain/vulcain.h"

int main(i32 argc, char **argv)
{
    INFO("Hello, World ! Welcome to vulcain !");
    
    vc_ctx ctx = {0};
    vc_create_ctx(&ctx, &(instance_desc){
        .app_name = "Playground",
        .engine_name = "Playground",
        .engine_version = VK_MAKE_VERSION(0, 0, 0),
        .enable_debugging = TRUE,
        .extension_count = 0,
        .extensions = NULL
    });

    return 0;
}