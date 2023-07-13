#include "vc_handles.h"
#include "vc_managed_types.h"
#include "vulcain.h"
#include <vulkan/vulkan_core.h>

b8              _vc_priv_semaphore_destroy(vc_ctx *ctx, vc_priv_man_semaphore *sem)
{
    vkDestroySemaphore(ctx->vk_device, sem->semaphore, NULL);
    return TRUE;
}

vc_semaphore    vc_semaphore_create(vc_ctx   *ctx)
{
    VkSemaphoreCreateInfo sem_ci =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkSemaphore sem;
    VK_CHECKH(vkCreateSemaphore(ctx->vk_device, &sem_ci, NULL, &sem), "Could not create a semaphore.");

    vc_priv_man_semaphore vc_sem =
    {
        .semaphore = sem,
    };
    vc_handle_mgr_set_destroy_func(&ctx->handle_manager, VC_HANDLE_SEMAPHORE, (vc_man_destroy_func)_vc_priv_semaphore_destroy);
    return vc_handle_mgr_write_alloc(&ctx->handle_manager, VC_HANDLE_SEMAPHORE, &vc_sem);
}