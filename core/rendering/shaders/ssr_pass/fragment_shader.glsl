#version 410 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D g_atmosphere;
uniform sampler2D g_albedospec;
uniform sampler2D g_depth;
uniform sampler2D g_water_depth;
uniform sampler2D g_water_normal;

uniform mat4 view;
uniform mat4 projection;

bool outside_screen_space(vec2 ray){
    return (ray.x < 0 || ray.x > 1 || ray.y < 0 || ray.y > 1);
}

vec4 trace_ray(vec3 origin, vec3 direction, int iteration_count){
    vec3 ray_pos = origin;
    vec4 hit_color = vec4(0.0F);

    for(int i = 0; i < iteration_count; i++){
        ray_pos += direction;

        if(outside_screen_space(ray_pos.xy)) {
            break;
        }

        float sample_depth = texture(g_depth, ray_pos.xy).r;
        float depth_diff = ray_pos.z - sample_depth;

        if((depth_diff < 0.00005F) && !(depth_diff < 0.0F)) {
            float diff = length(ray_pos - origin) * 1.5F;
            hit_color = vec4(texture(g_albedospec, ray_pos.xy).rgb, 1.0F);
            break;
        }
    }

    return hit_color;
}

void main(){
    const float max_distance = 64.0F;
    float frag_depth = texture(g_water_depth, TexCoords).r;

    vec3 frag_pos = vec3(TexCoords, frag_depth);
    vec3 frag_normal = transpose(inverse(mat3(view))) * texture(g_water_normal, TexCoords).rgb;
    frag_normal = normalize(frag_normal);

    vec4 position_view = inverse(projection) * vec4(frag_pos * 2.0F - vec3(1.0F), 1.0F);
    position_view.xyz /= position_view.w;

    vec3 reflection_view = reflect(position_view.xyz, frag_normal);
    reflection_view = normalize(reflection_view);

    if(reflection_view.z > 0.0F){
        FragColor = vec4(0.0F);
        return;
    }

    vec3 vs_ray_end = position_view.xyz + reflection_view * max_distance;

    vec4 ray_end_pos = projection * vec4(vs_ray_end, 1.0F);
    ray_end_pos /= ray_end_pos.w;
    ray_end_pos.xyz = ray_end_pos.xyz * 0.5F + 0.5F;

    vec3 direction = ray_end_pos.xyz - frag_pos;
    vec2 texture_size = textureSize(g_depth, 0);

    ivec2 ss_start_pos = ivec2(frag_pos.x * texture_size.x, frag_pos.y * texture_size.y);
    ivec2 ss_end_pos = ivec2(ray_end_pos.x * texture_size.x, ray_end_pos.y * texture_size.y);
    ivec2 ss_distance = ss_end_pos - ss_start_pos;

    int ss_max_distance = max(abs(ss_distance.x), abs(ss_distance.y)) / 2;
    direction /= max(ss_max_distance, 0.001F);

    FragColor = trace_ray(frag_pos, direction, ss_max_distance);
}