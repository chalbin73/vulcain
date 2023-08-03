/**
 * @file
 * @brief Format query code, to query formats based on requested features on device
 */

#include "vulcain.h"

VkFormat    vc_format_query(vc_ctx *ctx, format_query query)
{
    for(int i = 0; i < query.candidates.format_count; i++)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(ctx->vk_selected_physical_device, query.candidates.formats[i], &props);

        b8 valid = TRUE;

        valid &= ( (props.optimalTilingFeatures & query.required_optimal_tiling_features) == query.required_optimal_tiling_features );
        valid &= ( (props.linearTilingFeatures & query.required_linear_tiling_features) == query.required_linear_tiling_features );
        valid &= ( (props.bufferFeatures & query.required_buffer_features) == query.required_buffer_features );

        if(valid)
        {
            return query.candidates.formats[i];
        }
    }
    return 0;
}
