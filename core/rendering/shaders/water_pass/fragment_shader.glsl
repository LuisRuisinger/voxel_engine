#version 410 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

uniform sampler2D water_normal_tex;

in vec3 FragPos;
in vec2 FragNormal;

void main() {
    gPosition = FragPos;

    vec3 normal = texture(water_normal_tex, FragNormal).rgb;
    normal.r = normal.r * 2.0F - 1.0F;
    normal.g = normal.b;
    normal.b = normal.g * 2.0F - 1.0F;

    gNormal = normalize(normal);
}