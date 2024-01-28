#include <vulcain/femtolog.h>
#include <vulcain/vulcain.h>
#include <vulcain/vc_device.h>

uint64_t
device_score(void *ud, VkPhysicalDevice phy)
{
    return 1;
}

int
main(int argc, char **argv)
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

    vc_device_builder b = vc_device_builder_begin(&ctx);

    vc_device_builder_set_score_func(b, device_score, NULL);
    vc_device_builder_add_queue(b, VK_QUEUE_COMPUTE_BIT);
    vc_queue main = vc_device_builder_add_queue(b, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    vc_device_builder_add_queue(b, VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);

    vc_device_builder_request_extension(b, "VK_KHR_swapchain");

    vc_device_builder_end(b);

    (void)main;
    vc_ctx_destroy(&ctx);
    return 0;
}

