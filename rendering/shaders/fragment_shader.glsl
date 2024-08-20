#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in float Height;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
    //FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
    float normalizedHeight = (Height + 128.0) / 512.0 * 2.0;
    FragColor = vec4(vec3(normalizedHeight), 1.0);
}

