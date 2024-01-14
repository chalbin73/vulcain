#include <vulcain/femtolog.h>
#include <vulcain/vulcain.h>

int    main(int argc, char **argv)
{
    vc_ctx ctx =
    {
        0
    };
    vc_ctx_create(
        &ctx,
        (VkApplicationInfo)
        {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .apiVersion         = VK_MAKE_VERSION(1, 3, 0),
            .pEngineName        = "Waltuh",
            .pApplicationName   = "Waltuh",
            .engineVersion      = VK_MAKE_VERSION(4, 2, 0),
            .applicationVersion = VK_MAKE_VERSION(6, 9, 0)
        },
        NULL,
        TRUE,
        0,
        NULL,
        0,
        NULL
        );

    vc_ctx_destroy(&ctx);
    return 0;
}

