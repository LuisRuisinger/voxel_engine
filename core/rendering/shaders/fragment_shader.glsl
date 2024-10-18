#version 410 core
out vec4 FragColor;

in vec3 Texture;

uniform sampler2DArray texture_array;

void main() {
    FragColor = vec4(texture(texture_array, Texture).xyz, 1.0);
}

