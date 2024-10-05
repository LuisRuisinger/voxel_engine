#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

// geometry pass textures
uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedospec;
uniform sampler2D g_atmosphere;

// ssao pass texture
#ifdef SSAO_PASS
uniform sampler2D g_ssao;
#endif

// lighting pass uniforms for shading
uniform vec3 light_direction;
uniform vec3 view_direction;

const vec3 ambient_light = vec3(1.0F);
const vec3 sun_light = vec3(1.0F);

const float k_a = 0.35F;
const float k_d = 0.60F;
const float k_s = 0.15F;
const float s   = 32.0F;

// distance fog
const float density = 0.007F;
const float gradient = 1.5F;
const float no_fog_offset = 256.0F;

// color correction
const float gamma = 2.2F;
const float exposure = 1.0F;

void main() {
    vec3 frag_pos = texture(g_position, TexCoords).rgb;
    vec3 normal   = texture(g_normal, TexCoords).rgb;
    vec3 albedo   = texture(g_albedospec, TexCoords).rgb;

#ifdef SSAO_PASS
    float ambient_occlusion = texture(g_ssao, TexCoords).r;
#endif

    vec3 l = normalize(light_direction);
    vec3 n = normalize(normal);
    vec3 v = normalize(view_direction - frag_pos);
    vec3 h = normalize(v + l);

    float i_a = 1.0F;
    float i_s = pow(clamp(dot(n, h), 0.0, 1.0), s);
    float i_d = clamp(dot(n, l), 0.0, 1.0);

    vec3 ambient = i_a * k_a * albedo * ambient_light;
    vec3 specular = i_s * k_s * sun_light;
    vec3 diffuse = i_d * k_d * albedo * sun_light;

    vec3 final_color = vec3(0.0F);

    // TODO: this is very dirty but we can abuse the fact that normals are normalized
    if (length(normal) == 1.0F) {
        vec2 frag_pos_2d = frag_pos.xz;
        vec2 camera_pos_2d = view_direction.xz;

        float frag_distance_2d = distance(frag_pos_2d, camera_pos_2d);
        frag_distance_2d = frag_distance_2d - no_fog_offset;
        frag_distance_2d = max(frag_distance_2d, 0.0F);

        vec3 fragment_color = ambient + specular + diffuse;
        fragment_color = clamp(fragment_color, 0.0F, 1.0F);

        vec3 atmosphere_color = texture(g_atmosphere, TexCoords).rgb;

        float visiblity = exp(-1.0F * pow(frag_distance_2d * density, gradient));
        visiblity = clamp(visiblity, 0.0F, 1.0F);

        final_color = mix(atmosphere_color, fragment_color, visiblity);
    }
    else {
        final_color = texture(g_atmosphere, TexCoords).rgb;
    }

    // hdr tone mapping with gamma correction
    final_color = final_color / (final_color + vec3(1.0F));
    final_color = pow(final_color, vec3(gamma));
    final_color *= exposure;
    FragColor = vec4(final_color, 1.0F);
}