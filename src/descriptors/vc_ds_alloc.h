#ifndef __VC_DS_ALLOC__
#define __VC_DS_ALLOC__

#include <vulkan/vulkan.h>
#include "../vulcain.h"

// Based on vkguide's implementation

typedef struct
{
    VkDescriptorType    type;
    f32                 ratio;
} vc_ds_ratio;

typedef struct
{
    VkDescriptorPool   *ready_pools; // darray
    VkDescriptorPool   *full_pools; // darray

    vc_ds_ratio        *ratios; // darray
    u32                 current_set_count;
} vc_descriptor_set_allocator;

void            vc_ds_alloc_create(vc_descriptor_set_allocator *allocator, vc_ds_ratio *pool_ratios, u32 ratio_count, u32 start_set_count);

VkDescriptorSet vc_ds_alloc_allocate(vc_descriptor_set_allocator *allocator, VkDevice dev, VkDescriptorSetLayout layout);

void vc_ds_alloc_destroy(vc_descriptor_set_allocator *allocator, VkDevice dev);
#endif // __VC_DS_ALLOC__

