#include "vulcain.h"

/* ---------------- Swapchain ---------------- */
b8      _vc_priv_create_swapchain(vc_ctx *ctx, swapchain_desc desc, VkExtent2D extent);
b8      _vc_priv_select_swapchain_configuration(vc_ctx *ctx, swapchain_configuration_query query);
b8      _vc_priv_delete_swapchain(vc_ctx   *ctx);
void    vc_swapchain_commit(vc_ctx *ctx, swapchain_desc desc);
b8      _vc_priv_get_optimal_swapchain_size(vc_ctx *ctx, swapchain_desc desc, VkExtent2D *extent);
void    vc_swapchain_cleanup(vc_ctx   *ctx);
VkImageAspectFlags    vc_priv_format_infer_aspect_mask(VkFormat    input);
