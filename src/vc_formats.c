/**
 * @file
 * @brief Format query code, to query formats based on requested features on device
 */

#include "vulcain.h"

b8    vc_format_query_index(vc_ctx *ctx, format_query query, vc_format_set candidates, u32 *index)
{
    for(int i = 0; i < candidates.format_count; i++)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(ctx->vk_selected_physical_device, candidates.formats[i], &props);

        b8 valid = TRUE;

        valid &= ( (props.optimalTilingFeatures & query.required_optimal_tiling_features) == query.required_optimal_tiling_features );
        valid &= ( (props.linearTilingFeatures & query.required_linear_tiling_features) == query.required_linear_tiling_features );
        valid &= ( (props.bufferFeatures & query.required_buffer_features) == query.required_buffer_features );

        if(valid)
        {
            *index = i;
            return TRUE;
        }
    }
    return FALSE;
}

VkFormat    vc_format_query(vc_ctx *ctx, format_query query, vc_format_set candidates)
{
    u32 index = 0;
    if( vc_format_query_index(ctx, query, candidates, &index) )
    {
        return candidates.formats[index];
    }
    return FALSE;
}

