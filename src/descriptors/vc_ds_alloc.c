#include "vc_ds_alloc.h"
#include "../base/data_structures/darray.h"
#include "../vc_enum_util.h"

#define _VC_POOL_RESIZE_FACTOR   1.5f
#define _VC_POOL_MAX_SETS        4096

#define _VC_DEFAULT_RATIOS_COUNT 11
vc_ds_ratio _vc_ds_default_ratios[_VC_DEFAULT_RATIOS_COUNT] =
{ {
      VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f
  },
  {
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f
  },
  {
      VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f
  },
  {
      VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f
  },
  {
      VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f
  },
  {
      VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f
  },
  {
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f
  },
  {
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f
  },
  {
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f
  },
  {
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f
  },
  {
      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f
  } };

#include <alloca.h>

VkDescriptorPool
_vc_ds_get_pool(VkDevice dev, vc_descriptor_set_allocator *allocator)
{
    // Check if pool is available in ready pools
    if(darray_length(allocator->ready_pools) > 0)
    {
        VkDescriptorPool pool = VK_NULL_HANDLE;
        darray_pop(allocator->ready_pools, &pool);
        return pool;
    }

    // Create a new pool
    VkDescriptorPoolSize *sizes = alloca( darray_length(allocator->ratios) * sizeof(*sizes) );

    for(u32 i = 0; i < darray_length(allocator->ratios); i++)
    {
        sizes[i] = (VkDescriptorPoolSize)
        {
            .type            = allocator->ratios[i].type,
            .descriptorCount = allocator->ratios[i].ratio * allocator->current_set_count,
        };
    }

    allocator->current_set_count *= _VC_POOL_RESIZE_FACTOR;
    if(allocator->current_set_count > _VC_POOL_MAX_SETS)
    {
        allocator->current_set_count = _VC_POOL_MAX_SETS;
    }

    VkDescriptorPoolCreateInfo pool_ci =
    {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = allocator->current_set_count,
        .poolSizeCount = darray_length(allocator->ratios),
        .pPoolSizes    = sizes,
    };

    VkDescriptorPool new_pool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorPool(dev, &pool_ci, NULL, &new_pool), "Could not create a new descriptor pool.");

    return new_pool;
}

void
vc_ds_alloc_create(vc_descriptor_set_allocator *allocator, vc_ds_ratio *pool_ratios, u32 ratio_count, u32 start_set_count)
{
    allocator->full_pools        = darray_create(VkDescriptorPool);
    allocator->ready_pools       = darray_create(VkDescriptorPool);
    allocator->ratios            = darray_create(vc_ds_ratio);
    allocator->current_set_count = start_set_count;

    if(ratio_count == 0 || !pool_ratios)
    {
        pool_ratios = _vc_ds_default_ratios;
        ratio_count = _VC_DEFAULT_RATIOS_COUNT;
    }

    for(u32 i = 0; i < ratio_count; i++)
    {
        darray_push(allocator->ratios, pool_ratios[i]);
    }
}

VkDescriptorSet
vc_ds_alloc_allocate(vc_descriptor_set_allocator *allocator, VkDevice dev, VkDescriptorSetLayout layout)
{
    VkDescriptorPool pool = _vc_ds_get_pool(dev, allocator);

    VkDescriptorSetAllocateInfo alloc_info =
    {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = pool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &layout,
    };

    VkDescriptorSet ds = VK_NULL_HANDLE;
    VkResult res       = vkAllocateDescriptorSets(dev, &alloc_info, &ds);

    // Allocation failed, mark pool as full
    if(res == VK_ERROR_OUT_OF_POOL_MEMORY || res == VK_ERROR_FRAGMENTED_POOL)
    {
        darray_push(allocator->full_pools, pool);

        // Retry
        pool                      = _vc_ds_get_pool(dev, allocator);
        alloc_info.descriptorPool = pool;
        VK_CHECK(vkAllocateDescriptorSets(dev, &alloc_info, &ds), "Descriptor set allocation error: Free pool should not have been full.");
    }

    // Put gotten pull back into list
    darray_push(allocator->ready_pools, pool);
    return ds;
}

void
vc_ds_alloc_destroy(vc_descriptor_set_allocator *allocator, VkDevice dev)
{
    u32 ready_pool_length = darray_length(allocator->ready_pools);
    for(u32 i = 0; i < ready_pool_length; i++)
    {
        vkDestroyDescriptorPool(dev, allocator->ready_pools[i], NULL);
    }

    u32 full_pool_length = darray_length(allocator->full_pools);
    for(u32 i = 0; i < full_pool_length; i++)
    {
        vkDestroyDescriptorPool(dev, allocator->full_pools[i], NULL);
    }

    darray_destroy(allocator->full_pools);
    darray_destroy(allocator->ready_pools);
    darray_destroy(allocator->ratios);
}

