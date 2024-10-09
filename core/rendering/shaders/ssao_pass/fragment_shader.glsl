#version 330 core

out float FragColor;

in vec2 TexCoords;

uniform sampler2D g_depth;
uniform mat4 view;
uniform mat4 projection;

#define SAMPLES_AMOUNT 8
#define NOISE_AMOUNT 8

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

vec3 normal_from_depth(float depth, vec3 point) {
    float offset_height = 1.0F / textureSize(g_depth, 0).y;
    float offset_width  = 1.0F / textureSize(g_depth, 0).x;

    float depth_height = texture(g_depth, TexCoords + vec2(0.0F, offset_height)).r;
    float depth_width  = texture(g_depth, TexCoords + vec2(offset_width, 0.0F)).r;

    vec3 point_1 = vec3(offset_width, depth_height - depth, 0.0F);
    vec3 point_2 = vec3(offset_height, depth_height - depth, 0.0F);

    vec3 normal = cross(point_1, point_2);
    normal.z = -normal.z;

    return normalize(normal);
}

void main() {
    const float falloff = 0.000001F;
    const float area = 0.0075F;
    const float total_strength = 1.0F;
    const float radius = 0.35F;
    const float bias = 0.025F;
    const float magnitude = 0.75F;
    const float contrast = 1.0F;

    float frag_depth = texture(g_depth, TexCoords).r;
    vec3 frag_pos = vec3(TexCoords, frag_depth);
    vec3 frag_normal = normal_from_depth(frag_depth, frag_pos);

    // noise for tbn matrix
    int noise_s = int(sqrt(NOISE_AMOUNT));
    int noise_x = int(TexCoords.x * textureSize(g_depth, 0).x) % noise_s;
    int noise_y = int(TexCoords.y * textureSize(g_depth, 0).y) % noise_s;

    vec3 random = noise[noise_x + (noise_y * noise_s)];
    random = normalize(random);

    float radius_depth = radius / frag_depth;
    float occlusion = 0.0F;

    for (int i = 0; i < SAMPLES_AMOUNT; ++i) {
        vec3 ray = radius_depth * reflect(samples[i], random);
        vec3 hemi_ray = frag_pos + sign(dot(ray, frag_normal)) * ray;

        float occ_depth = texture(g_depth, clamp(hemi_ray.xy, 0.0F, 1.0F)).r;
        float difference = frag_depth - occ_depth;

        occlusion += step(falloff, difference) * (1.0F - smoothstep(falloff, area, difference));
    }

    float ao = 1.0F - total_strength * occlusion * (1.0F / SAMPLES_AMOUNT);
    ao = clamp(ao, 0.0F, 1.0F);
    ao = pow(ao, magnitude);

    FragColor = ao;
}