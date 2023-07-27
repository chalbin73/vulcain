#version 450

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.43, 0.25),
    vec2(-0.43, 0.25)
);

vec3 colors[3] = vec3[](
    vec3(1, 0, 0),
    vec3(0, 1.0, 0),
    vec3(0, 0, 1)
);

layout (location = 0) out vec2 uv;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec3 frag_pos;

layout (location = 0) in vec3 vert_pos;
layout (location = 1) in vec3 vert_nor;
layout (location = 2) in vec2 vert_uv;

layout (set = 0, binding = 0) uniform ubo
{
    mat4 model_mat;
    mat4 view_mat;
    mat4 prj_mat;
};

vec3 light_pos = vec3(0, 1.0f, 0);
vec3 cam_pos = vec3(0, 0, 0);

void main() {

    mat4 model_view = view_mat * model_mat;
    gl_Position = prj_mat * model_view * vec4(vert_pos, 1.0);

    uv = vert_uv;

    normal = (transpose(inverse(model_view)) * vec4(vert_nor, 0.0)).xyz;

    frag_pos = gl_Position.xyz;

}
