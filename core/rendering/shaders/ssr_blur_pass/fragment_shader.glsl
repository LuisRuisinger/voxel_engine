#version 410 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D g_ssr;

void main() {
    const int radius = 1;
    const float volume = pow(float(radius + 1), 2.0F);

    vec2 texel_size = 1.0F / textureSize(g_ssr, 0);
    vec4 result = vec4(0.0F);

    for (int x = -radius; x <= radius; ++x) {
        for (int y = -radius; y <= radius; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            vec4 tex = texture(g_ssr, TexCoords + offset).rgba;

            // results in stability through holes
            result.rgb += tex.rgb * tex.a;
            result.a += tex.a;
        }
    }

    FragColor = result / volume;
}