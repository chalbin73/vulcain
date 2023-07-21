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

layout (location = 0) out vec3 color;

layout (location = 0) in vec3 vert_pos;

layout (set = 0, binding = 0) uniform ubo
{
    mat4 prj;
};

void main() {

    vec4 new_pos = vec4(vert_pos, 1.0);
    new_pos.z -= 0.5f;
    gl_Position = prj * new_pos;
}
