#version 410 core

out float FragColor;

in vec2 TexCoords;

uniform sampler2D g_ssao;

void main() {
    const int radius = 1;
    const float volume = pow(float(radius + 1), 2.0F);

    vec2 texel_size = 1.0F / textureSize(g_ssao, 0).xy;
    float result = 0.0F;

    for (int x = -radius; x <= radius; ++x) {
        for (int y = -radius; y <= radius; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            result += texture(g_ssao, TexCoords + offset).r;
        }
    }

    FragColor = result / volume;
}