#include "vulcain.h"

// Internal functions

/* ---------------- Swapchain ---------------- */
b8                       _vc_priv_create_swapchain(vc_ctx *ctx, swapchain_desc desc, VkExtent2D extent);
b8                       _vc_priv_select_swapchain_configuration(vc_ctx *ctx, swapchain_configuration_query query);
b8                       _vc_priv_delete_swapchain(vc_ctx   *ctx);
void                     vc_swapchain_commit(vc_ctx *ctx, swapchain_desc desc);
b8                       _vc_priv_get_optimal_swapchain_size(vc_ctx *ctx, swapchain_desc desc, VkExtent2D *extent);
void                     vc_swapchain_cleanup(vc_ctx   *ctx);
VkImageAspectFlags       vc_priv_format_infer_aspect_mask(VkFormat    input);


#define VC_PRIV_DESCRIPTOR_POOL_ALLOCATOR_ALLOC_COUNT    100
#define VC_PRIV_DESCRIPTOR_SET_LAYOUT_CACHE_BUCKET_COUNT 17

// Descriptor set allocator functions

/**
 * @brief Creates a descriptor set allocator
 *
 * @param ctx
 * @param alloc The new allocator
 */
void                     vc_priv_descriptor_set_allocator_create(vc_ctx *ctx, vc_descriptor_set_allocator *alloc);

/**
 * @brief Allocates a descriptor set
 *
 * @param ctx
 * @param alloc
 * @param layout The layout with which to create the descriptor set
 * @param set The newly created set
 * @param parent_pool A handle to the pool that allocated the set
 * @return
 */
b8                       vc_priv_descriptor_set_allocator_alloc(vc_ctx *ctx, vc_descriptor_set_allocator *alloc, VkDescriptorSetLayout layout, VkDescriptorSet *set, VkDescriptorPool *parent_pool);

/**
 * @brief Resets all pools of descriptors
 *
 * @param ctx
 * @param alloc
 */
void                     vc_priv_descriptor_set_allocator_reset(vc_ctx *ctx, vc_descriptor_set_allocator *alloc);

/**
 * @brief Destroys a descriptor set allocator, as well as the subsequent vulkan objects
 *
 * @param ctx
 * @param alloc
 */
void                     vc_priv_descriptor_set_allocator_destroy(vc_ctx *ctx, vc_descriptor_set_allocator *alloc);

// Descriptor set layout cache functions

/**
 * @brief Creates a descriptor set layout cache
 *
 * @param cache 
 */
void                     vc_priv_descriptor_set_layout_cache_create(vc_descriptor_set_layout_cache   *cache);

/**
 * @brief Grabs a descriptor set layout : creates it if necessary and reuses them
 *
 * @param ctx 
 * @param cache 
 * @param info The create info
 * @return 
 */
VkDescriptorSetLayout    vc_priv_descriptor_set_layout_cache_grab(vc_ctx *ctx, vc_descriptor_set_layout_cache *cache, VkDescriptorSetLayoutCreateInfo info);

/**
 * @brief Destroys the cache, destroys vkDescriptorSetLayouts
 *
 * @param ctx 
 * @param cache 
 */
void                     vc_priv_descriptor_set_layout_cache_destroy(vc_ctx *ctx, vc_descriptor_set_layout_cache *cache);
