#version 330 core

out float FragColor;

in vec2 TexCoords;

uniform sampler2D g_position;
uniform sampler2D g_normal;

uniform mat4 view;
uniform mat4 projection;

const float radius = 0.25F;
const float bias = 0.05F;
const float magnitude = 1.10F;
const float contrast = 1.10F;

#define SAMPLES_AMOUNT 16
#define NOISE_AMOUNT 16

const vec3 samples[SAMPLES_AMOUNT] = vec3[](
    vec3(-0.062265, -0.051678, 0.037445), vec3(0.031211, -0.020697, 0.017174),
    vec3(-0.044208, 0.045405, 0.008349), vec3(0.012702, 0.047817, 0.045715),
    vec3(0.078975, 0.103812, 0.076138), vec3(-0.049897, -0.018186, 0.110603),
    vec3(0.126449, 0.145343, 0.025935), vec3(0.070281, -0.035581, 0.080867),
    vec3(-0.178339, 0.013866, 0.184085), vec3(0.019236, 0.004503, 0.030125),
    vec3(0.128037, -0.079219, 0.350573), vec3(-0.002789, 0.099905, 0.013120),
    vec3(-0.076747, 0.077850, 0.011658), vec3(0.064317, 0.043618, 0.203733),
    vec3(-0.102557, -0.147598, 0.762084), vec3(0.046845, -0.204307, 0.212321)
);

const vec3 noise[NOISE_AMOUNT] = vec3[](
    vec3(-0.088435, -0.847564, 0.000000), vec3(-0.742405, -0.650952, 0.000000),
    vec3(-0.116445, -0.925400, 0.000000), vec3(0.008327, -0.065170, 0.000000),
    vec3(0.158032, 0.348225, 0.000000), vec3(-0.813177, -0.866593, 0.000000),
    vec3(0.665405, -0.220435, 0.000000), vec3(-0.618273, -0.669203, 0.000000),
    vec3(0.917093, 0.981609, 0.000000), vec3(-0.772871, 0.741278, 0.000000),
    vec3(0.244502, 0.345305, 0.000000), vec3(0.228091, 0.175412, 0.000000),
    vec3(-0.698193, -0.489971, 0.000000), vec3(0.597878, 0.186091, 0.000000),
    vec3(0.793303, -0.456560, 0.000000), vec3(-0.604099, -0.459038, 0.000000)
);

void main() {
    // g_position is in world space from the geometry pass
    vec3 frag_pos = texture(g_position, TexCoords).xyz;

    // g_normal is in world space form the geometry pass
    vec3 normal = texture(g_normal, TexCoords).xyz;
    normal = normalize(-normal);

    // noise for tbn matrix
    int noise_s = int(sqrt(NOISE_AMOUNT));
    int noise_x = int(TexCoords.x) % noise_s;
    int noise_y = int(TexCoords.y) % noise_s;
    vec3 random = noise[noise_x + (noise_y * noise_s)];

    // create tbn matrix
    vec3 tangent = normalize(random - normal * dot(random, normal));
    vec3 binormal = normalize(cross(normal, tangent));
    mat3 tbn = mat3(tangent, binormal, normal);

    float occlusion = SAMPLES_AMOUNT;
    for (int i = 0; i < SAMPLES_AMOUNT; ++i) {
        vec3 sample_pos = tbn * samples[i];
        sample_pos = frag_pos + sample_pos * radius;

        vec4 offset_uv = projection * view * vec4(sample_pos, 1.0F);
        offset_uv.xyz /= offset_uv.w;
        offset_uv.xyz = offset_uv.xyz * 0.5F + 0.5F;

        vec3 offset_pos = texture(g_position, offset_uv.xy).xyz;

        float occluded = float(offset_pos.x > sample_pos.x + bias);
        //float intensity = smoothstep(0.0F, 1.0F, radius / abs(frag_pos.z - offset_pos.z));
        float intensity = 1.0F;

        occluded *= intensity;
        occlusion -= occluded;
    }

    occlusion /= SAMPLES_AMOUNT;
    occlusion = pow(occlusion, magnitude);
    occlusion = contrast * (occlusion - 0.5F) + 0.5F;

    FragColor = occlusion;
}