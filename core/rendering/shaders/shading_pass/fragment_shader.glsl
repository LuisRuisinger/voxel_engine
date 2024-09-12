#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedospec;

void main() {
    vec3 frag_pos = texture(g_position, TexCoords).rgb;
    vec3 normal = texture(g_normal, TexCoords).rgb;

    vec4 tmp = texture(g_albedospec, TexCoords);
    vec3 diffuse = tmp.rgb;
    float specular = tmp.a;

    // TODO: actual lighting calc
    FragColor = vec4(diffuse, 1.0);
}