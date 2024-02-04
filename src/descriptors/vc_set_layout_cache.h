#ifndef __VC_SET_LAYOUT_CACHE__
#define __VC_SET_LAYOUT_CACHE__

#include <vulkan/vulkan.h>
#include "../base/types.h"

#define VC_SLC_BUCKET_COUNT 17

typedef struct
{
    VkDescriptorSetLayout    layout;
    u64                      hash;
} _vc_slc_cell;

typedef struct
{
    _vc_slc_cell   *buckets[VC_SLC_BUCKET_COUNT]; // darray
} vc_set_layout_cache;

void
vc_slc_create(vc_set_layout_cache   *cache);

VkDescriptorSetLayout vc_slc_get(vc_set_layout_cache *cache, VkDevice dev, VkDescriptorSetLayoutCreateInfo info);

void                  vc_slc_destroy(vc_set_layout_cache *cache, VkDevice dev);


#endif // __VC_SET_LAYOUT_CACHE__

