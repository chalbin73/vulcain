#include "handles/vc_internal_types.h"
#include "vulcain.h"
#include "vc_enum_util.h"

void
_vc_semaphore_destroy(vc_ctx *ctx, _vc_semaphore_intern *s)
{
    vkDestroySemaphore(ctx->current_device, s->semaphore, NULL);
}

vc_semaphore
vc_semaphore_create(vc_ctx   *ctx)
{
    VkSemaphoreCreateInfo sem_ci =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    _vc_semaphore_intern sem_intern =
    {
        0
    };
    VK_CHECKH(vkCreateSemaphore(ctx->current_device, &sem_ci, NULL, &sem_intern.semaphore), "Semaphore creation failed.");
    sem_intern.is_timeline = FALSE;

    vc_semaphore hndl = vc_handles_manager_walloc(&ctx->handles_manager, VC_HANDLE_SEMAPHORE, &sem_intern);
    vc_handles_manager_set_destroy_function(&ctx->handles_manager, VC_HANDLE_SEMAPHORE, (vc_handle_destroy_func)_vc_semaphore_destroy);

    return hndl;
}

