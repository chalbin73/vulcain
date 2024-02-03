#ifndef __VC_SET_LAYOUT_CACHE__
#define __VC_SET_LAYOUT_CACHE__

#include "../vulcain.h"
#include "../base/allocators/pool_allocator.h"

#define VC_SLC_BUCKET_COUNT 17

typedef struct _vc_slc_cell
{
    VkDescriptorSetLayout              layout;
    VkDescriptorSetLayoutCreateInfo    info;
    struct _vc_slc_cell               *next;
} _vc_slc_cell;

typedef struct _vc_slc_binding_cell
{
    VkDescriptorSetLayoutBinding    binding;
    struct _vc_slc_cell            *next;
}_vc_slc_binding_cell;

typedef struct
{
    mem_pool        set_layouts_pool;
    mem_pool        bindings_pool;

    _vc_slc_cell   *buckets[VC_SLC_BUCKET_COUNT];
} vc_set_layout_cache;

void                  vc_slc_create(vc_set_layout_cache *cache, u32 set_layout_pool_size, u32 binding_pool_size);

VkDescriptorSetLayout vc_slc_get(vc_set_layout_cache *cache, VkDescriptorSetLayoutCreateInfo info);

#endif // __VC_SET_LAYOUT_CACHE__

