#include "vulcain.h"

/* ---------------- Swapchain ---------------- */
b8    _vc_priv_create_swapchain(vc_ctx *ctx, VkExtent2D extent);
b8    _vc_priv_select_swapchain_configuration(vc_ctx   *ctx);
b8    _vc_priv_delete_swapchain(vc_ctx   *ctx);
b8    _vc_priv_setup_default_swapchain(vc_ctx   *ctx);
b8    _vc_priv_get_optimal_swapchain_size(vc_ctx *ctx, VkExtent2D *extent);