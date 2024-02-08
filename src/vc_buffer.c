#include "vulcain.h"
#include "handles/vc_internal_types.h"
#include "vc_enum_util.h"

void
_vc_buffer_destory(vc_ctx *ctx, _vc_buffer_intern *b)
{
    vmaDestroyBuffer(ctx->main_allocator, b->buffer, b->alloc);
}

vc_buffer
vc_buffer_allocate(vc_ctx *ctx, u64 size, VkBufferCreateFlags flags, VkBufferUsageFlags usage, vc_memory_create_info mem)
{
    VkBufferCreateInfo buf_ci =
    {
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size                  = size,
        .flags                 = flags,
        .usage                 = usage,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE, // TODO: Add a concurrent option
        .queueFamilyIndexCount = 0,
    };

    VmaAllocationCreateInfo alloc_ci =
    {
        .usage         = mem.usage,
        .flags         = mem.flags,
        .requiredFlags = mem.mem_props,
    };

    _vc_buffer_intern buf_i =
    {
        0
    };

    VK_CHECKH(vmaCreateBuffer(ctx->main_allocator, &buf_ci, &alloc_ci, &buf_i.buffer, &buf_i.alloc, NULL), "Could not allocate a buffer");
    buf_i.size = size;

    vc_buffer hndl = vc_handles_manager_walloc(&ctx->handles_manager, VC_HANDLE_BUFFER, &buf_i);
    vc_handles_manager_set_destroy_function(&ctx->handles_manager, VC_HANDLE_BUFFER, (vc_handle_destroy_func)_vc_buffer_destory);

    return hndl;
}

