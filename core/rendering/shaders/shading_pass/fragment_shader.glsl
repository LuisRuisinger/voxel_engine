#version 410 core
precision highp float;

out vec4 FragColor;

in vec2 TexCoords;

// geometry pass textures
uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedospec;
uniform sampler2D g_depth;

uniform sampler2D g_atmosphere;
uniform sampler2DArray g_depth_map;
uniform sampler2D g_ssao;

// water pass textures
uniform sampler2D g_water;
uniform sampler2D g_water_normal;
uniform sampler2D g_water_depth;

// lighting pass uniforms for shading
uniform vec3 light_direction;
uniform vec3 view_direction;

const vec3 ambient_light = vec3(1.0F);
const vec3 sun_light = vec3(1.0F);

const float k_a = 0.35F;
const float k_d = 0.75F;
const float k_s = 0.55F;

// distance fog
const float density = 0.007F;
const float gradient = 1.5F;

// TODO: make this dynamic
const float no_fog_offset = 256.0F;

const vec3 water_color = vec3(0.0392f, 0.3627f, 0.6392f);

vec3 light_gradient() {
    float s = smoothstep(-0.025F, 0.075F, light_direction.y);

    return mix(vec3(0.0F), sun_light, s);
}

uniform mat4 view;
uniform int cascade_count;
uniform float cascade_plane_distances[4];
uniform float far_z;

layout (std140) uniform LSMatrices
{
    mat4 ls_matrices[4];
};

float shadow_calculation(vec3 ws_coords, vec3 normal) {
    vec4 vs_coords = view * vec4(ws_coords, 1.0F);
    float depth_value = abs(vs_coords.z);

    int layer = cascade_count - 1;
    for (int i = cascade_count - 2; i > -1; --i) {
        if (depth_value < cascade_plane_distances[i])
            layer = i;
    }

    vec4 light_coords = ls_matrices[layer] * vec4(ws_coords, 1.0F);

    light_coords.xyz = light_coords.xyz / light_coords.w;
    light_coords.xyz = light_coords.xyz * 0.5F + 0.5F;
    if (light_coords.z > 1.0F)
            return 0.0F;

    float bias = max(0.005F * (1.0F - dot(normal, -light_direction)), 0.05F);
    bias *= 1 / (cascade_plane_distances[layer] * 0.5F);

    float current_depth = light_coords.z;
    float shadow = 0.0F;

    // pcf
    vec2 texel_size = 1.0F / textureSize(g_depth_map, 0).xy;
    for(int x = -1; x < 2; ++x) {
        for(int y = -1; y < 2; ++y)  {
            vec3 pcf_coords = vec3(light_coords.xy + vec2(x, y) * texel_size, layer);
            float pcf_closest_depth = texture(g_depth_map, pcf_coords).r;
            shadow += float(current_depth - bias > pcf_closest_depth);
        }
    }

    shadow /= 9.0F;
    return shadow;
}

// hdr tone mapping with gamma correction
vec3 hdr(vec3 color) {
    const float gamma = 2.2F;
    const float exposure = 1.0F;

    color = color / (color + vec3(1.0F));
    color = pow(color, vec3(gamma));
    color *= exposure;

    return color;
}

#define NEAR_PLANE 0.1F
#define FAR_PLANE 640.0F
#define M_PI 3.1415926535897932384626433832795

float linearize_depth(float depth) {
    float z_n = 2.0 * depth - 1.0;
    return 2.0 * NEAR_PLANE * FAR_PLANE / (FAR_PLANE + NEAR_PLANE - z_n * (FAR_PLANE - NEAR_PLANE));
}

vec3 screen_space_reflections(vec3 frag_pos, vec3 frag_normal) {
    return vec3(0.0F);
}

vec3 combined_color(vec3 frag_pos, vec3 frag_normal, vec3 frag_color, float ao_factor, float s) {
    vec3 l = normalize(light_direction);
    vec3 n = normalize(frag_normal);
    vec3 v = normalize(view_direction - frag_pos);
    vec3 h = normalize(v + l);

    float i_a = 1.0F;
    float i_s = pow(clamp(dot(n, h), 0.0, 1.0), s);
    float i_d = clamp(dot(n, l), 0.0, 1.0);

    vec3 gradient_light = light_gradient();

    // shadow peel
    float shadow = shadow_calculation(frag_pos, frag_normal);

    vec3 ambient = i_a * k_a * frag_color * ambient_light;
    ambient *= max(smoothstep(-0.025F, 0.075F, light_direction.y), 0.3);
    ambient *= ao_factor;

    vec3 specular = i_s * k_s * gradient_light;
    specular = specular * (1.0F - shadow);

    vec3 diffuse = i_d * k_d * frag_color * gradient_light;
    diffuse = diffuse * (1.0F - shadow);

    return ambient + specular + diffuse;
}

void main() {
    vec3 frag_pos          = texture(g_position, TexCoords).rgb;
    vec3 frag_normal       = texture(g_normal, TexCoords).rgb;
    vec3 frag_color        = texture(g_albedospec, TexCoords).rgb;
    float frag_world_depth = texture(g_depth, TexCoords).r;

    vec3 frag_water        = texture(g_water, TexCoords).rgb;
    float frag_water_depth = texture(g_water_depth, TexCoords).r;

    float ambient_occlusion = texture(g_ssao, TexCoords).r;

    frag_color = combined_color(frag_pos, frag_normal, frag_color, ambient_occlusion, 32.0F);

    vec3 final_color = vec3(0.0F);
    if (length(frag_normal) == 1.0F) {
        vec3 atmosphere_color = texture(g_atmosphere, TexCoords).rgb;

        if (frag_water_depth < frag_world_depth) {
            float linearized_depth = linearize_depth(frag_world_depth);
            float linearized_water_depth = linearize_depth(frag_water_depth);
            float linearized_depth_diff = linearized_depth - linearized_water_depth;

            float falloff = (linearize_depth(frag_world_depth) - linearize_depth(frag_water_depth)) / 48.0F;
            falloff = clamp(falloff, 0.0F, 1.0F);

            vec3 frag_water_normal = texture(g_water_normal, TexCoords).xyz;
            frag_color = mix(
                water_color,
                frag_color,
                1.0F - falloff);


            vec3 frag_water_color = combined_color(
                frag_water,
                frag_water_normal,
                water_color,
                1.0F,
                80.0F);

            // fresnel
            float fresnel = 0.8F - clamp(dot(-view_direction, frag_water_normal), 0.2F, 0.8F);
            frag_color = mix(frag_water_color, frag_color, 0.4);
        }

        float frag_distance_2d = distance(frag_pos.xz, view_direction.xz);
        frag_distance_2d = frag_distance_2d - no_fog_offset;
        frag_distance_2d = max(frag_distance_2d, 0.0F);

        float visiblity = exp(-1.0F * pow(frag_distance_2d * density, gradient));
        visiblity = clamp(visiblity, 0.0F, 1.0F);

        final_color = mix(atmosphere_color, frag_color, visiblity);
    }
    else {
        final_color = texture(g_atmosphere, TexCoords).rgb;
    }

    FragColor = vec4(hdr(final_color), 1.0F);
}