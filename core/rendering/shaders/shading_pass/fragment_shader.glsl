#version 330 core
precision highp float;

out vec4 FragColor;

in vec2 TexCoords;

// geometry pass textures
uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedospec;
uniform sampler2D g_atmosphere;
uniform sampler2D g_depth_map;

#define SSAO_PASS

// ssao pass texture
#ifdef SSAO_PASS
uniform sampler2D g_ssao;
#endif

// lighting pass uniforms for shading
uniform vec3 light_direction;
uniform vec3 view_direction;

// transformation matrices
uniform mat4 depth_map_view;
uniform mat4 depth_map_projection;

const vec3 ambient_light = vec3(1.0F);
const vec3 sun_light = vec3(1.0F);

const float k_a = 0.35F;
const float k_d = 0.75F;
const float k_s = 0.15F;
const float s   = 32.0F;

// distance fog
const float density = 0.007F;
const float gradient = 1.5F;
const float no_fog_offset = 256.0F;

vec3 light_gradient() {
    float s = smoothstep(-0.025F, 0.075F, light_direction.y);

    return mix(vec3(0.0F), sun_light, s);
}

float shadow_calculation(vec4 light_coords, vec3 normal) {
    float shadow_bias = max(0.05F * (1.0F - dot(normal, light_direction)), 0.005F);

    light_coords.xyz = light_coords.xyz / light_coords.w;
    light_coords.xyz = light_coords.xyz * 0.5F + 0.5F;

    if (light_coords.z > 1.0F)
            return 0.0F;

    float current_depth = light_coords.z;
    float shadow = 0.0F;

    // pcf
    vec2 texel_size = 1.0F / textureSize(g_depth_map, 0);
    for(int x = -1; x < 2; ++x) {
        for(int y = -1; y < 2; ++y)  {
            vec2 pcf_coords = light_coords.xy + vec2(x, y) * texel_size;
            float pcf_closest_depth = texture(g_depth_map, pcf_coords).r;
            shadow += float(current_depth - shadow_bias > pcf_closest_depth);
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

void main() {
    vec3 frag_pos           = texture(g_position, TexCoords).rgb;
    vec3 normal             = texture(g_normal, TexCoords).rgb;
    vec3 albedo             = texture(g_albedospec, TexCoords).rgb;
    float ambient_occlusion = texture(g_ssao, TexCoords).r;

    vec3 l = normalize(light_direction);
    vec3 n = normalize(normal);
    vec3 v = normalize(view_direction - frag_pos);
    vec3 h = normalize(v + l);

    float i_a = 1.0F;
    float i_s = pow(clamp(dot(n, h), 0.0, 1.0), s);
    float i_d = clamp(dot(n, l), 0.0, 1.0);

    vec3 gradient_light = light_gradient();

    vec3 ambient = i_a * k_a * albedo * ambient_light;
    ambient *= max(smoothstep(-0.025F, 0.075F, light_direction.y), 0.3);
    ambient *= ambient_occlusion;
    ambient *= max(0.0F, 1.0F - i_d * k_d);

    vec3 specular = i_s * k_s * gradient_light;
    vec3 diffuse = i_d * k_d * albedo * gradient_light;

    // shadow peel
    vec4 light_coords = depth_map_projection * depth_map_view * vec4(frag_pos, 1.0F);
    float shadow = shadow_calculation(light_coords, normal);

    specular = specular * (1.0F - shadow);
    diffuse = diffuse * (1.0F - shadow);

    vec3 final_color = vec3(0.0F);

    // TODO: this is very dirty but we can abuse the fact that normals are normalized
    if (length(normal) == 1.0F) {
        vec3 fragment_color = ambient + specular + diffuse;
        vec3 atmosphere_color = texture(g_atmosphere, TexCoords).rgb;

        float frag_distance_2d = distance(frag_pos.xz, view_direction.xz);
        frag_distance_2d = frag_distance_2d - no_fog_offset;
        frag_distance_2d = max(frag_distance_2d, 0.0F);

        float visiblity = exp(-1.0F * pow(frag_distance_2d * density, gradient));
        visiblity = clamp(visiblity, 0.0F, 1.0F);

        final_color = mix(atmosphere_color, fragment_color, visiblity);
    }
    else {
        final_color = texture(g_atmosphere, TexCoords).rgb;
    }

    FragColor = vec4(hdr(final_color), 1.0F);
}