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

layout (set = 0, binding = 0) uniform ubo
{
    float rotation;
};

void main() {
    vec2 pos = positions[gl_VertexIndex];
    mat2 rot;

    rot[0] = vec2(cos(rotation), sin(rotation));
    rot[1] = vec2(-sin(rotation), cos(rotation));

    pos = rot * pos;

    gl_Position = vec4(pos, 0.0, 1.0);
    color = colors[gl_VertexIndex];
}