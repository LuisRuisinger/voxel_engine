#version 410 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;

uniform sampler2D water_normal_tex;

in vec3 FragPos;
in vec2 FragNormal;

void main() {
    const float wave_strength = 0.1F;
    gPosition = FragPos;

    vec3 normal = texture(water_normal_tex, FragNormal / 16.0F).rgb;
    normal.r = (normal.r * 2.0F - 1.0F) * wave_strength;
    normal.g = normal.b;
    normal.b = (normal.g * 2.0F - 1.0F) * wave_strength;

    gNormal = normalize(normal);
}