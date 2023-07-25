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
layout (location = 1) in vec3 vert_nor;
layout (location = 2) in vec2 vert_uv;

layout (set = 0, binding = 0) uniform ubo
{
    mat4 prj;
    float time;
};

void main() {
    mat2 rot;
    rot[0] = vec2(cos(time), sin(time));
    rot[1] = vec2(-sin(time), cos(time));

    vec2 hp = vec2(vert_pos.x, vert_pos.y);
    hp = rot * hp;

    vec3 new_pos = vec3(hp, vert_pos.z);
    new_pos *= 0.4f;
    new_pos.z -= 0.15f;
    new_pos.x -= 0.1f;
    gl_Position = prj * vec4(new_pos, 1.0);

    color = vec3(vert_uv, 0.0);
}
