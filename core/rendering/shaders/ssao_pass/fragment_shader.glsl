#version 410 core

out float FragColor;

in vec2 TexCoords;

uniform sampler2D g_depth;
uniform sampler2D g_normal;
uniform mat4 view;
uniform mat4 projection;

#define SAMPLES_AMOUNT 16
#define NOISE_AMOUNT 16

const vec3 samples[16] = vec3[](
    vec3(-0.062265, -0.051678, 0.037445), vec3(0.031211, -0.020697, 0.017174),
    vec3(-0.044208, 0.045405, 0.008349), vec3(0.012702, 0.047817, 0.045715),
    vec3(0.078975, 0.103812, 0.076138), vec3(-0.049897, -0.018186, 0.110603),
    vec3(0.126449, 0.145343, 0.025935), vec3(0.070281, -0.035581, 0.080867),
    vec3(-0.178339, 0.013866, 0.184085), vec3(0.019236, 0.004503, 0.030125),
    vec3(0.128037, -0.079219, 0.350573), vec3(-0.002789, 0.099905, 0.013120),
    vec3(-0.076747, 0.077850, 0.011658), vec3(0.064317, 0.043618, 0.203733),
    vec3(-0.102557, -0.147598, 0.762084), vec3(0.046845, -0.204307, 0.212321)
);

const vec3 noise[16] = vec3[](
    vec3(-0.088435, -0.847564, 0.000000), vec3(-0.742405, -0.650952, 0.000000),
    vec3(-0.116445, -0.925400, 0.000000), vec3(0.008327, -0.065170, 0.000000),
    vec3(0.158032, 0.348225, 0.000000), vec3(-0.813177, -0.866593, 0.000000),
    vec3(0.665405, -0.220435, 0.000000), vec3(-0.618273, -0.669203, 0.000000),
    vec3(0.917093, 0.981609, 0.000000), vec3(-0.772871, 0.741278, 0.000000),
    vec3(0.244502, 0.345305, 0.000000), vec3(0.228091, 0.175412, 0.000000),
    vec3(-0.698193, -0.489971, 0.000000), vec3(0.597878, 0.186091, 0.000000),
    vec3(0.793303, -0.456560, 0.000000), vec3(-0.604099, -0.459038, 0.000000)
);

vec3 normal_from_depth(float depth, vec2 tex_coords) {
    vec2 texel_size = 1.0F / textureSize(g_depth, 0);

    float depth_right = texture(g_depth, tex_coords + vec2(texel_size.x, 0.0F)).r;
    float depth_left = texture(g_depth, tex_coords - vec2(texel_size.x, 0.0F)).r;
    float depth_up = texture(g_depth, tex_coords + vec2(0.0F, texel_size.y)).r;
    float depth_down = texture(g_depth, tex_coords - vec2(0.0F, texel_size.y)).r;

    // gradients
    vec3 dx = vec3(2.0 * texel_size.x, depth_right - depth_left, 0.0F);
    vec3 dy = vec3(0.0F, depth_up - depth_down, 2.0 * texel_size.y);

    vec3 normal = normalize(-1.0F * cross(dx, dy));
    return normal;
}

void main() {
    const float falloff = 0.000001F;
    const float area = 0.0075F;
    const float total_strength = 1.0F;
    const float radius = 0.35F;
    const float magnitude = 0.75F;
    const float contrast = 1.0F;

    float frag_depth = texture(g_depth, TexCoords).r;
    if (frag_depth < 0.0F) {
        FragColor = 1.0F;
        return;
    }

    vec3 frag_pos = vec3(TexCoords, frag_depth);
    vec3 frag_normal = transpose(inverse(mat3(view))) * texture(g_normal, TexCoords).xyz;
    frag_normal = normalize(frag_normal);

    // noise for TBN matrix
    int noise_s = int(sqrt(NOISE_AMOUNT));
    int noise_x = int(TexCoords.x * textureSize(g_depth, 0).x) % noise_s;
    int noise_y = int(TexCoords.y * textureSize(g_depth, 0).y) % noise_s;

    vec3 random = noise[noise_x + (noise_y * noise_s)];
    random = normalize(random);

    vec3 tangent = normalize(random - frag_normal * dot(random, frag_normal));
    vec3 bitangent = cross(frag_normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, frag_normal);

    float radius_depth = radius / frag_depth;
    float occlusion = 0.0F;

    for (int i = 0; i < SAMPLES_AMOUNT; ++i) {
        vec3 sample_dir = TBN * samples[i];
        vec3 sample_pos = frag_pos + radius_depth * sample_dir;

        float occ_depth = texture(g_depth, clamp(sample_pos.xy, 0.0F, 1.0F)).r;
        float difference = frag_depth - occ_depth;

        occlusion += step(falloff, difference) * (1.0F - smoothstep(falloff, area, difference));
    }

    float ao = 1.0F - total_strength * occlusion * (1.0F / SAMPLES_AMOUNT);
    ao = clamp(ao, 0.0F, 1.0F);
    ao = pow(ao, magnitude);

    FragColor = ao;
}
