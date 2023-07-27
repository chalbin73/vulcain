#version 450

layout(location = 0) out vec4 out_color;

layout (location = 0) in vec2 uv;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 frag_pos;

layout (binding = 1) uniform sampler2D samp;

vec3 light_pos = vec3(0, -1.0f, 5.0f);
vec3 cam_pos = vec3(0, 0, 0);

void main() {
    vec4 tex_color = vec4(texture(samp, uv));

    float light = dot(light_pos, normal);

    vec3 demi_refl = ((cam_pos - frag_pos) + (light_pos - frag_pos)) / 2;
    demi_refl = normalize(demi_refl);

    float spec = pow(max(dot(demi_refl, normal), 0.0f), 8);
    
    vec3 reflected_ray = reflect(normalize(cam_pos - frag_pos), normal);
    reflected_ray = normalize(reflected_ray);

    spec = pow(max(dot(reflected_ray, normalize(light_pos - frag_pos)), 0.0f), 8);

    out_color = max(light, 0) * tex_color + spec * 0.2 * vec4(1.0);
    out_color = vec4(spec);


}
