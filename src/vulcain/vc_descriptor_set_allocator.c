#include "vulcain.h"
#include "vc_private.h"

/*
 *
 * Based on VkGuide's implementation :
 * https://vkguide.dev/docs/extra-chapter/abstracting_descriptors/
 *
 */

#define _VC_PRIV_POOL_SIZE_COUNT 11
struct
{
    VkDescriptorType    type; f32 weight;
}
_pool_sizes[_VC_PRIV_POOL_SIZE_COUNT] =
{ { VK_DESCRIPTOR_TYPE_SAMPLER,                0.5f },
  { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f  },
  { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          4.f  },
  { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1.f  },
  { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1.f  },
  { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1.f  },
  { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         2.f  },
  { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         2.f  },
  { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f  },
  { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f  },
  { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       0.5f } };

void                vc_priv_descriptor_set_allocator_create(vc_ctx *ctx, vc_descriptor_set_allocator *alloc)
{
    alloc->free_pools = darray_create(VkDescriptorPool);
    alloc->used_pools = darray_create(VkDescriptorPool);

    alloc->current_pool = NULL;
}

VkDescriptorPool    _vc_priv_create_pool(vc_ctx *ctx, u32 set_count, VkDescriptorPoolCreateFlags flags)
{
    VkDescriptorPoolSize sizes[_VC_PRIV_POOL_SIZE_COUNT];

    for(int i = 0; i < _VC_PRIV_POOL_SIZE_COUNT; i++)
    {
        sizes[i] = (VkDescriptorPoolSize)
        {
            .type            = _pool_sizes[i].type,
            .descriptorCount = _pool_sizes[i].weight * set_count,
        };
    }

    VkDescriptorPoolCreateInfo pool_ci =
    {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = flags,
        .maxSets       = set_count,
        .poolSizeCount = _VC_PRIV_POOL_SIZE_COUNT,
        .pPoolSizes    = sizes,
    };

    VkDescriptorPool desc_pool;
    VK_CHECK(vkCreateDescriptorPool(ctx->vk_device, &pool_ci, NULL, &desc_pool), "Could not create a descriptor pool");
    return desc_pool;
}

VkDescriptorPool    _vc_priv_grab_pool(vc_ctx *ctx, vc_descriptor_set_allocator *alloc)
{
    u32 free_count = darray_length(alloc->free_pools);
    if(free_count > 0)
    {
        VkDescriptorPool pool;
        darray_pop(alloc->free_pools, &pool);
        return pool;
    }

    return _vc_priv_create_pool(ctx, VC_PRIV_DESCRIPTOR_POOL_ALLOCATOR_ALLOC_COUNT, 0);
}

b8    vc_priv_descriptor_set_allocator_alloc(vc_ctx *ctx, vc_descriptor_set_allocator *alloc, VkDescriptorSetLayout layout, VkDescriptorSet *set)
{
    if(alloc->current_pool == VK_NULL_HANDLE)
    {
        alloc->current_pool = _vc_priv_grab_pool(ctx, alloc);
        darray_push(alloc->used_pools, alloc->current_pool);
    }

    VkDescriptorSetAllocateInfo alloc_i =
    {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorSetCount = 1,
        .pSetLayouts        = &layout,
        .descriptorPool     = alloc->current_pool,
    };

    VkResult res = vkAllocateDescriptorSets(ctx->vk_device, &alloc_i, set);

    switch (res)
    {
    case VK_SUCCESS:
        return TRUE;

    case VK_ERROR_FRAGMENTED_POOL:
    case VK_ERROR_OUT_OF_POOL_MEMORY:
        break;

    default:
        return FALSE;

    }
    // Need reallocation
    alloc->current_pool = _vc_priv_grab_pool(ctx, alloc);
    darray_push(alloc->used_pools, alloc->current_pool);

    alloc_i.descriptorPool = alloc->current_pool;

    res = vkAllocateDescriptorSets(ctx->vk_device, &alloc_i, set);

    if(res == VK_SUCCESS)
    {
        return TRUE;
    }
    return FALSE;
}

void    vc_priv_descriptor_set_allocator_reset(vc_ctx *ctx, vc_descriptor_set_allocator *alloc)
{
    u32 used_count = darray_length(alloc->used_pools);
    for(int i = 0; i < used_count; i++)
    {
        vkResetDescriptorPool(ctx->vk_device, alloc->used_pools[i], 0);
        darray_push(alloc->free_pools, alloc->used_pools[i]);
    }

    darray_destroy(alloc->used_pools);
    alloc->used_pools = darray_create(VkDescriptorPool);

    alloc->current_pool = VK_NULL_HANDLE;
}

void    vc_priv_descriptor_set_allocator_destroy(vc_ctx *ctx, vc_descriptor_set_allocator *alloc)
{
    u32 used_count = darray_length(alloc->used_pools);
    for(int i = 0; i < used_count; i++)
    {
        vkDestroyDescriptorPool(ctx->vk_device, alloc->used_pools[i], NULL);
    }

    u32 free_count = darray_length(alloc->free_pools);
    for(int i = 0; i < free_count; i++)
    {
        vkDestroyDescriptorPool(ctx->vk_device, alloc->free_pools[i], NULL);
    }
    darray_destroy(alloc->free_pools);

    alloc->current_pool = VK_NULL_HANDLE;
}
