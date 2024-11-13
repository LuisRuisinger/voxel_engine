#version 410 core
precision highp float;

#define NEAR_PLANE 0.1F
#define FAR_PLANE 640.0F
#define M_PI 3.1415926535897932384626433832795

out vec4 FragColor;
in vec2 TexCoords;

// geometry pass textures
uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedospec;
uniform sampler2D g_depth;

// atmosphere
uniform sampler2D g_atmosphere;

// ssao
uniform sampler2D g_ssao;

// water pass textures
uniform sampler2D g_water;
uniform sampler2D g_water_normal;
uniform sampler2D g_water_depth;

// lighting pass uniforms for shading
uniform vec3 light_direction;
uniform vec3 view_direction;

// distance fog
uniform uint render_radius;

// camera view
uniform mat4 view;

// shadow depth map
uniform sampler2DArray g_depth_map;
uniform int cascade_count;
uniform float cascade_plane_distances[4];
uniform float far_z;

// ssr
uniform sampler2D g_albedospec_ssr;

// light space matrices
layout (std140) uniform LSMatrices {
    mat4 ls_matrices[4];
};

vec3 light_gradient() {
    const vec3 sun_light = vec3(1.0F);

    float s = smoothstep(-0.025F, 0.075F, light_direction.y);
    return mix(vec3(0.0F), sun_light, s);
}

float shadow_calculation(vec3 ws_coords, vec3 normal) {
    vec4 vs_coords = view * vec4(ws_coords, 1.0F);
    float depth_value = abs(vs_coords.z);

    int layer = cascade_count - 1;
    for (int i = cascade_count - 2; i > -1; --i) {
        if (depth_value < cascade_plane_distances[i]) {
            layer = i;
        }
    }

    vec4 light_coords = ls_matrices[layer] * vec4(ws_coords, 1.0F);
    light_coords.xyz = light_coords.xyz / light_coords.w;
    light_coords.xyz = light_coords.xyz * 0.5F + 0.5F;

    if (light_coords.z > 1.0F) {
        return 0.0F;
    }

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

float linearize_depth(float depth) {
    float z_n = 2.0 * depth - 1.0;
    return 2.0 * NEAR_PLANE * FAR_PLANE / (FAR_PLANE + NEAR_PLANE - z_n * (FAR_PLANE - NEAR_PLANE));
}

vec3 combined_color(vec3 frag_pos, vec3 frag_normal, vec3 frag_color, float ao_factor, float s) {
    const float k_a = 0.35F;
    const float k_d = 0.65F;
    const float k_s = 0.35F;
    const vec3 ambient_light = vec3(1.0F);

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

vec3 combined_water_color(vec3 frag_color, float frag_world_depth) {
    const vec3 water_color = vec3(0.0392f, 0.3627f, 0.6392f);

    float frag_water_depth = texture(g_water_depth, TexCoords).r;
    if (!(frag_water_depth < frag_world_depth)) {
        return frag_color;
    }

    float linearized_depth = linearize_depth(frag_world_depth);
    float linearized_water_depth = linearize_depth(frag_water_depth);
    float linearized_depth_diff = linearized_depth - linearized_water_depth;

    float falloff = linearize_depth(frag_world_depth) - linearize_depth(frag_water_depth);
    falloff /= 48.0F;
    falloff = clamp(falloff, 0.0F, 0.8F);

    vec3 frag_water_normal = texture(g_water_normal, TexCoords).xyz;
    vec3 frag_water        = texture(g_water, TexCoords).rgb;
    vec4 reflection_color  = texture(g_albedospec_ssr, TexCoords).rgba;

    // fresnel
    vec3 v = normalize(view_direction - frag_water);
    float fresnel = clamp(dot(v, frag_water_normal), 0.1F, 0.9F);

    float scale = (0.9F - fresnel) * (1.0F - reflection_color.a);
    clamp(scale, 0.1F, 0.9F);

    vec3 frag_water_color = mix(water_color, reflection_color.rgb, scale);
    frag_water_color = combined_color(frag_water, frag_water_normal, frag_water_color, 1.0F, 80.0F);

    frag_color = mix(water_color, frag_color, 0.8F - falloff);
    frag_color = mix(frag_water_color, frag_color, fresnel);
    return frag_color;
}

vec3 combined_fog_color(vec3 frag_color, vec3 frag_pos, float frag_world_depth) {
    const float density = 0.007F;
    const float gradient = 1.5F;

    vec3 atmosphere_color = texture(g_atmosphere, TexCoords).rgb;
    if (frag_world_depth == 1.0F) {
        return atmosphere_color;
    }

    float no_fog_offset = max(float(render_radius) / 2.0F, 4.0F) * 32.0F;

    float frag_distance_2D = distance(frag_pos.xz, view_direction.xz);
    frag_distance_2D = frag_distance_2D - no_fog_offset;
    frag_distance_2D = max(frag_distance_2D, 0.0);

    float visiblity = exp(-1.0F * pow(frag_distance_2D * density, gradient));
    visiblity = clamp(visiblity, 0.0F, 1.0F);

    frag_color = mix(atmosphere_color, frag_color, visiblity);
    return frag_color;
}

void main() {
    const vec3 water_color = vec3(0.0392F, 0.3627F, 0.6392F);

    vec3 frag_pos           = texture(g_position, TexCoords).rgb;
    vec3 frag_normal        = texture(g_normal, TexCoords).rgb;
    vec3 frag_color         = texture(g_albedospec, TexCoords).rgb;
    float frag_world_depth  = texture(g_depth, TexCoords).r;
    float ambient_occlusion = texture(g_ssao, TexCoords).r;

    frag_color = combined_color(frag_pos, frag_normal, frag_color, ambient_occlusion, 16.0F);
    frag_color = combined_water_color(frag_color, frag_world_depth);
    frag_color = combined_fog_color(frag_color, frag_pos, frag_world_depth);
    frag_color = hdr(frag_color);

    FragColor = vec4(frag_color, 1.0F);
}
