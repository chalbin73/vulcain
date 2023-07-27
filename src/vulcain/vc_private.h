#include "vulcain.h"

// Internal functions

/* ---------------- Swapchain ---------------- */
b8                    _vc_priv_create_swapchain(vc_ctx *ctx, swapchain_desc desc, VkExtent2D extent);
b8                    _vc_priv_select_swapchain_configuration(vc_ctx *ctx, swapchain_configuration_query query);
b8                    _vc_priv_delete_swapchain(vc_ctx   *ctx);
void                  vc_swapchain_commit(vc_ctx *ctx, swapchain_desc desc);
b8                    _vc_priv_get_optimal_swapchain_size(vc_ctx *ctx, swapchain_desc desc, VkExtent2D *extent);
void                  vc_swapchain_cleanup(vc_ctx   *ctx);
VkImageAspectFlags    vc_priv_format_infer_aspect_mask(VkFormat    input);


#define VC_PRIV_DESCRIPTOR_POOL_ALLOCATOR_ALLOC_COUNT 100

typedef struct
{
    VkDescriptorPool   *used_pools; // darray
    VkDescriptorPool   *free_pools; // darray

    VkDescriptorPool    current_pool;
} vc_descriptor_set_allocator;

void    vc_priv_descriptor_set_allocator_create(vc_ctx *ctx, vc_descriptor_set_allocator *alloc);
b8      vc_priv_descriptor_set_allocator_alloc(vc_ctx *ctx, vc_descriptor_set_allocator *alloc, VkDescriptorSetLayout layout, VkDescriptorSet *set);
void    vc_priv_descriptor_set_allocator_reset(vc_ctx *ctx, vc_descriptor_set_allocator *alloc);
void    vc_priv_descriptor_set_allocator_destroy(vc_ctx *ctx, vc_descriptor_set_allocator *alloc);
