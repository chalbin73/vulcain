#include "vulcain/vc_handles.h"
#include <vulkan/vulkan_core.h>
#define VC_ENABLE_WINDOWING_GLFW 1

#include "base/base.h"
#include "vulcain/vulcain.h"
#include <GLFW/glfw3.h>
#include <fast_obj.h>
#include <mathc.h>
#include <math.h>

#include <stb_image.h>

GLFWwindow *window;

vc_framebuffer *frambuffers;
VkExtent2D fb_size;

typedef struct
{
    f32    pos[3];
    f32    nor[3];
    f32    uv[2];
} vertex;

vc_image depth_buffer;
vc_image_view depth_buffer_view;
vc_semaphore sem;
void    swp_recreation_cllbck(vc_ctx *ctx, void *data, swapchain_created_info info)
{
    depth_buffer = vc_image_allocate(
        ctx,
        (image_create_desc)
        {
            .width           = info.width,
            .height          = info.height,
            .depth           = 1,
            .share           = FALSE,
            .layout          = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .image_dimension = 2,
            .sample_count    = VK_SAMPLE_COUNT_1_BIT,
            .image_usage     = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .image_format    = vc_swapchain_configuration_get(ctx).depth_format,
        }
        );

    INFO( "Creating depth buffer with : %s", vc_priv_VkFormat_to_str(vc_swapchain_configuration_get(ctx).depth_format) );

    depth_buffer_view = vc_image_view_create(
        ctx,
        depth_buffer,
        (image_view_desc)
        {
            .view_type                        = VK_IMAGE_VIEW_TYPE_2D,
            .component_mapping                = VC_COMPONENT_MAPPING_ID,
            .subresource_range.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
            .subresource_range.layerCount     = 1,
            .subresource_range.levelCount     = 1,
            .subresource_range.baseMipLevel   = 0,
            .subresource_range.baseArrayLayer = 0,
        }
        );

    frambuffers = mem_allocate(sizeof(vc_framebuffer) * info.image_count, MEMORY_TAG_RENDERER);
    for (int i = 0; i < info.image_count; i++)
    {
        render_attachments_set render_att =
        {
            .attachment_count = 2,
            .attachments      = (vc_image_view[2]){ vc_swapchain_get_image_view_hndls(ctx)[i], depth_buffer_view }
        };

        frambuffers[i] = vc_framebuffer_create(
            ctx,
            (framebuffer_desc){
                .attachment_set = render_att,
                .render_pass    = *( (vc_render_pass *)data ),
                .layers         = 1,
            }
            );
    }

    fb_size = (VkExtent2D)
    {
        .width = info.width, .height = info.height
    };

    sem = vc_semaphore_create(ctx);

}

void    swp_destruction_cllbck(vc_ctx *ctx, void *data)
{
    mem_free(frambuffers);
}

int     main(i32 argc, char **argv)
{
    INFO("/* ---------------- START ---------------- */");
    INFO("Hello, World ! Welcome to vulcain !");

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(1920, 1080, "Hello !", NULL, NULL);
    glfwShowWindow(window);
    u32 exts_count    = 0;
    const char **exts = glfwGetRequiredInstanceExtensions(&exts_count);

    f32 projection_matrix[MAT4_SIZE];
    f32 model_matrix[MAT4_SIZE];
    f32 view_matrix[MAT4_SIZE];

    mat4_identity(model_matrix);
    mat4_identity(view_matrix);
    mat4_perspective(projection_matrix, 3.1415f / 2.0f, 16.0f / 9.0f, 0.01f, 1000.0f);

    timer t = TIMER_START();

    vc_ctx ctx =
    {
        0
    };
    vc_create_ctx(
        &ctx,
        &(instance_desc){
            .app_name         = "Playground",
            .engine_name      = "Playground",
            .engine_version   = VK_MAKE_VERSION(0, 0, 0),
            .enable_debugging = FALSE,
            .extension_count  = exts_count,
            .extensions       = (char **)exts,
            .enable_windowing = TRUE,
            .windowing_system = vc_windowing_system_glfw(window),
        },
        &(physical_device_query){
            .allowed_types          = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU | VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
            .requested_features     = { .geometryShader = TRUE },
            .request_main_queue     = TRUE,
            .request_compute_queue  = TRUE,
            .request_transfer_queue = FALSE
        }
        );

    vc_swapchain_setup(
        &ctx,
        (swapchain_configuration_query){ }
        );

    vc_render_pass pass = vc_render_pass_create(
        &ctx,
        (render_pass_desc){
            .attachment_count = 2,
            .attachment_desc  = (render_pass_attachment_params[2]){
                [0] =
                {
                    .load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .store_op = VK_ATTACHMENT_STORE_OP_STORE,

                    .stencil_load_op  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE,

                    .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .final_layout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

                    .attachment_format        = vc_swapchain_configuration_get(&ctx).swapchain_format.format,
                    .attachment_sample_counts = VK_SAMPLE_COUNT_1_BIT,
                },
                [1] =
                {

                    .load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE,

                    .stencil_load_op  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE,

                    .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .final_layout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,

                    .attachment_format        = vc_swapchain_configuration_get(&ctx).depth_format,
                    .attachment_sample_counts = VK_SAMPLE_COUNT_1_BIT,
                },
            },

            .subpass_count  = 1,
            .subpasses_desc = (subpass_desc[1]){[0] =
                                                {
                                                    .pipline_type                 = VC_PIPELINE_TYPE_GRAPHICS,
                                                    .input_attachment_count       = 0,
                                                    .color_attachment_count       = 1,
                                                    .color_attachment_refs        = &(VkAttachmentReference){ .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL         },
                                                    .depth_stencil_attachment_ref = &(VkAttachmentReference){ .attachment = 1,.layout                  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL },
                                                    .preserve_attachment_count    = 0,
                                                } },

            .subpass_dependency_count = 1,
            .subpass_dependencies     = &(subpass_dependency_desc){
                .src_id = VK_SUBPASS_EXTERNAL,
                .dst_id = 0,

                .src_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dst_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,

                .src_access = 0,
                .dst_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            }
        }
        );

    vc_swapchain_commit(
        &ctx,
        (swapchain_desc){
            .swapchain_images_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .callback_user_data     = (void *)&pass,
            .recreation_callback    = swp_recreation_cllbck,
            .destruction_callack    = swp_destruction_cllbck,
        }
        );

    vc_buffer mvp_buf = vc_buffer_allocate(
        &ctx,
        (buffer_alloc_desc){
            .buffer_usage        = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .allocation_flags    = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .size                = sizeof(projection_matrix) * 4,
            .required_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .share               = FALSE,
        }
        );


    fastObjMesh *mesh = fast_obj_read("test_res/rat.obj");

    vc_buffer index_buffer = vc_buffer_allocate(
        &ctx,
        (buffer_alloc_desc)
        {
            .size                = sizeof(u32) * mesh->index_count,
            .buffer_usage        = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            .share               = FALSE,
            .required_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            .allocation_flags    = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
        );

    vc_buffer vertex_buffer = vc_buffer_allocate(
        &ctx,
        (buffer_alloc_desc)
        {
            .size                = sizeof(vertex) * mesh->position_count,
            .buffer_usage        = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .share               = FALSE,
            .required_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            .allocation_flags    = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
        );


    vertex *verts = mem_allocate(mesh->position_count * sizeof(vertex), MEMORY_TAG_RENDERER);
    u32 *indices  = mem_allocate(sizeof(u32) * mesh->index_count, MEMORY_TAG_RENDERER);
    for(int i = 0; i < mesh->index_count; i++)
    {
        indices[i] = mesh->indices[i].p;

        verts[indices[i]].pos[0] = mesh->positions[indices[i] * 3];
        verts[indices[i]].pos[1] = mesh->positions[indices[i] * 3 + 1];
        verts[indices[i]].pos[2] = mesh->positions[indices[i] * 3 + 2];

        verts[indices[i]].nor[0] = mesh->normals[mesh->indices[i].n * 3];
        verts[indices[i]].nor[1] = mesh->normals[mesh->indices[i].n * 3 + 1];
        verts[indices[i]].nor[2] = mesh->normals[mesh->indices[i].n * 3 + 2];

        verts[indices[i]].uv[0] = mesh->texcoords[mesh->indices[i].t * 2];
        verts[indices[i]].uv[1] = 1 - mesh->texcoords[mesh->indices[i].t * 2 + 1];
    }

    u32 index_count = mesh->index_count;
    TRACE("Wrinting indices");
    vc_buffer_coherent_staged_write(&ctx, index_buffer, 0, mesh->index_count * sizeof(u32), indices, VC_QUEUE_MAIN);
    TRACE("Wrinting vertices");
    vc_buffer_coherent_staged_write(&ctx, vertex_buffer, 0, mesh->position_count * sizeof(vertex), verts, VC_QUEUE_MAIN);

    TRACE("Mesh loaded");
//fast_obj_destroy(mesh);

    i32 tex_w, tex_h, tex_c = 0;
    u8 *data                = stbi_load("test_res/rat.png", &tex_w, &tex_h, &tex_c, 4);

    ASSERT(tex_c == 4);
    INFO("Image is %dx%d c%d", tex_w, tex_h, tex_c);

    vc_image rat_texture = vc_image_allocate(
        &ctx,
        (image_create_desc)
        {
            .width           = tex_w,
            .depth           = 1,
            .height          = tex_h,
            .share           = FALSE,
            .image_usage     = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .layout          = VK_IMAGE_LAYOUT_UNDEFINED,
            .image_format    = VK_FORMAT_R8G8B8A8_UNORM,
            .image_dimension = 2,
            .sample_count    = VK_SAMPLE_COUNT_1_BIT,
            .mip_levels      = 1,
            .layer_count     = 1,
        }
        );

    vc_buffer tex_stage = vc_buffer_allocate(
        &ctx,
        (buffer_alloc_desc)
        {
            .size                = sizeof(u8) * tex_w * tex_h * tex_c,
            .buffer_usage        = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocation_flags    = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .required_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        }
        );

    vc_buffer_coherent_staged_write(&ctx, tex_stage, 0, sizeof(u8) * tex_w * tex_h * tex_c, data, VC_QUEUE_MAIN);
    vc_image_fill_from_buffer(&ctx, rat_texture, tex_stage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VC_QUEUE_MAIN);

    vc_image_sampler sampler = vc_image_sampler_create(
        &ctx,
        (sampler_desc)
        {

            .mag_filter               = VK_FILTER_NEAREST,
            .min_filter               = VK_FILTER_NEAREST,
            .mipmap_mode              = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .address_mode_u           = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .address_mode_v           = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .address_mode_w           = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .anisotropy_enable        = VK_FALSE,
            .unnormalized_coordinates = VK_FALSE,
        }
        );

    descriptor_set_desc desc_set_desc =
    {
        .binding_count = 2,
        .bindings      = (descriptor_binding_desc[2])
        {
            [0] =
            {
                .descriptor_type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptor_count = 1,
                .stage_flags      = VK_SHADER_STAGE_VERTEX_BIT,
                .buffer_info      = &(descriptor_binding_buffer)
                {
                    .buffer       = mvp_buf,
                    .whole_buffer = TRUE,
                },
            },

            [1] = (descriptor_binding_desc)
            {
                .descriptor_type  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptor_count = 1,
                .stage_flags      = VK_SHADER_STAGE_FRAGMENT_BIT,
                .image_info       = &(descriptor_binding_image)
                {
                    .image_view                 = vc_image_view_create(
                        &ctx,
                        rat_texture,
                        (image_view_desc)
                        {
                            .view_type         = VK_IMAGE_VIEW_TYPE_2D,
                            .component_mapping = VC_COMPONENT_MAPPING_ID,
                            .subresource_range =
                            {
                                .baseArrayLayer = 0,
                                .layerCount     = 1,
                                .baseMipLevel   = 0,
                                .levelCount     = 1,
                                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                            },
                        }
                        ),
                    .layout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .sampler = sampler,
                }
            }

        }
    };

    vc_buffer_coherent_staged_write(&ctx, mvp_buf, 0, sizeof(projection_matrix), projection_matrix, VC_QUEUE_MAIN);

    vc_descriptor_set_layout set_layout = vc_descriptor_set_layout_create(&ctx, desc_set_desc);
    vc_descriptor_set set               = vc_descriptor_set_create(&ctx, desc_set_desc, set_layout);

    graphics_pipeline_code_desc code;
    code.vertex_code          = fio_read_whole_file("shaders/a.vert.spv", &code.vertex_code_size);
    code.vertex_entry_point   = "main";
    code.fragment_code        = fio_read_whole_file("shaders/a.frag.spv", &code.fragment_code_size);
    code.fragment_entry_point = "main";

    vc_graphics_pipe graphics_pipe = vc_graphics_pipe_create(
        &ctx,
        (graphics_pipeline_desc){
            .shader_code                = code,
            .vertex_input_binding_count = 1,
            .vertex_input_bindings      = &(graphics_pipeline_in_binding)
            {
                .stride          = sizeof(f32) * 8,
                .input_rate      = VK_VERTEX_INPUT_RATE_VERTEX,
                .attribute_count = 3,
                .attributes      = (graphics_pipeline_in_attrib[3])
                {

                    [0] =
                    {
                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                        .offset = 0,
                    },

                    [1] =
                    {
                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                        .offset = 3 * sizeof(f32),
                    },

                    [2] =
                    {
                        .format = VK_FORMAT_R32G32_SFLOAT,
                        .offset = 6 * sizeof(f32),

                    }
                }
            },
            .set_layout_count       = 1,
            .set_layouts            = &set_layout,
            .render_pass            = pass,
            .subpass_index          = 0,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .cull_mode              = VK_CULL_MODE_NONE,
            .front_face             = VK_FRONT_FACE_CLOCKWISE,
            .line_width             = 1.0f,
            .viewport_scissor_count = 1,
            .depth_test             = TRUE,
            .depth_write            = TRUE,
            .depth_compare_op       = VK_COMPARE_OP_LESS,
            .stencil_test           = FALSE,
            .enable_depth_clamp     = FALSE,
            .enable_depth_bias      = FALSE,
            .sample_count           = VK_SAMPLE_COUNT_1_BIT,
            .sample_shading         = FALSE,
            .attchment_count        = 1,
            .attchment_blends       = &(VkPipelineColorBlendAttachmentState){
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .colorBlendOp        = VK_BLEND_OP_ADD,
                .alphaBlendOp        = VK_BLEND_OP_ADD,
                .blendEnable         = VK_TRUE,
                .colorWriteMask      = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,
            },
            .blend_constants     = { 1, 1, 1, 1 },
            .dynamic_state_count = 2,
            .dynamic_states      = (VkDynamicState[2]){ VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT },
        }
        );
    (void)graphics_pipe;

    vc_command_buffer buf = vc_command_buffer_main_create(&ctx, VC_QUEUE_MAIN, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    TIMER_LOG(t, "Vulkan init");

    b8 recreated = FALSE;

    void *matrices = mem_allocate(sizeof(projection_matrix) * 3, MEMORY_TAG_RENDERER);
    //u64 time       = platform_millis();
    f32 v[VEC3_SIZE];

    while ( !glfwWindowShouldClose(window) )
    {
        /*
        u64 now = platform_millis();
        if ( (now - time) < 16 )
            continue;

        time = now;
        f32 t = (f32)time / 1000.0f;
        (void)t;
        */
        mat4_identity(model_matrix);
        mat4_rotation_z(model_matrix, t);
        mat4_translate( model_matrix, model_matrix, vec3(v, 0, 0, -0.3) );

        vc_queue_wait_idle(&ctx, VC_QUEUE_COMPUTE);
        vc_queue_wait_idle(&ctx, VC_QUEUE_MAIN);

        mem_memcpy( matrices, model_matrix, sizeof(model_matrix) );
        mem_memcpy( (void *)( (u64)matrices + sizeof(model_matrix) ), view_matrix, sizeof(model_matrix) );
        mem_memcpy( (void *)( (u64)matrices + (sizeof(model_matrix) * 2) ), projection_matrix, sizeof(model_matrix) );

        vc_buffer_coherent_staged_write(&ctx, mvp_buf, 0, sizeof(projection_matrix) * 3, matrices, VC_QUEUE_MAIN);

        u32 iid = 0;
        if ( !vc_swapchain_acquire_image(&ctx, &iid, sem) )
        {
            continue;
        }
        // vc_image curi = vc_swapchain_get_image_hndls(&ctx)[iid];

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !recreated)
        {
            recreated = TRUE;
            vc_swapchain_force_recreation(&ctx);
            continue;
        }

        vc_command_buffer_reset(&ctx, buf);
        vc_command_buffer_begin(&ctx, buf);

        vc_command_render_pass_begin(
            &ctx,
            buf,
            (render_pass_begin_desc){
                .pass              = pass,
                .render_area       = (VkRect2D){ { 0, 0 }, fb_size },
                .subpass_contents  = VK_SUBPASS_CONTENTS_INLINE,
                .clear_value_count = 2,
                .clear_values      = (VkClearValue[2]){
                    [0] = { .color = { { 1, 1, 1, 1 } } },
                    [1] = { .depthStencil = { .depth = 1.0f, .stencil = 0 } }
                },
                .frambuffer = frambuffers[iid],
            }
            );

        vc_command_dyn_set_viewport(
            &ctx,
            buf,
            1,
            &(VkViewport){ .x = 0, .y = 0, .width = fb_size.width, .height = fb_size.height, .minDepth = 0.0f, .maxDepth = 1.0f }
            );

        vc_command_dyn_set_scissors(
            &ctx,
            buf,
            1,
            &(VkRect2D){
                { 0, 0 },
                fb_size
            }
            );

        vc_command_pipeline_bind(&ctx, buf, graphics_pipe);
        vc_command_buffer_bind_descriptor_set(&ctx, buf, graphics_pipe, set);

        vc_command_bind_vertex_buffer(&ctx, buf, vertex_buffer, 0, 0);
        vc_command_bind_index_buffer(&ctx, buf, index_buffer, 0, VK_INDEX_TYPE_UINT32);

        vc_command_draw_indexed(&ctx, buf, index_count, 1, 0, 0, 0);

        vc_command_render_pass_end(&ctx, buf);

        vc_command_buffer_end(&ctx, buf);
        vc_command_buffer_submit(
            &ctx,
            buf,
            sem,
            (VkPipelineStageFlags[1]){[0] = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT }
            );
        vc_queue_wait_idle(&ctx, VC_QUEUE_MAIN);

        vc_swapchain_present_image(&ctx, iid);

        glfwPollEvents();
    }
    vc_destroy_ctx(&ctx);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
