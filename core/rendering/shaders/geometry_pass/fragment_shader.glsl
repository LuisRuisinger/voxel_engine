#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 FragNormal;
in vec3 FragTexture;

uniform sampler2DArray texture_array;

#define NEAR_PLANE 0.1F
#define FAR_PLANE 1024.0F

float linearize_depth(float depth, float near, float far)
{
    return (2.0 * near * far) / (far + near - depth * (far - near));
}

void main() {
    gPosition = FragPos;
    gNormal = normalize(FragNormal);
    gAlbedoSpec = vec4(texture(texture_array, FragTexture).xyz, 1.0);
    // gAlbedoSpec = vec4(vec3(0.9F), 1.0F);
}