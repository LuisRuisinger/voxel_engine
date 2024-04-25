#version 330 core

layout (location = 0) in vec3 pos;

out vec2 TexCoord;

uniform samplerBuffer textureBuffer;

uniform vec2 worldbase;
uniform mat4 view;
uniform mat4 projection;

vec2 texCoords[4] = vec2[4](
        vec2(0.0f, 0.0f),
        vec2(1.0f, 0.0f),
        vec2(1.0f, 1.0f),
        vec2(0.0f, 1.0f)
);

void main()  {
    //gl_Position = projection * view * vec4((os_pos * scale) + ls_pos, 1.0f);
    //TexCoord = vec2(tx_pos.x, 1.0 - tx_pos.y);

    vec3 basepos = vec3(worldbase.x, 0, worldbase.y);
    gl_Position = projection * view * vec4(basepos + pos, 1.0f);

    vec2 uv = texCoords[0];
    TexCoord = vec2(uv.x, 1.0 - uv.y);

    /*
    float xChunk = 0.0;
    float zChunk = 0.0;

    float xChunkOffset = float((block1 >> 7) & 0x1Fu);
    float zChunkOffset = float((block1 >> 2) & 0x1Fu);

    float yPos = float((block2 >> 22) & 0x3FFu);

    vec3 ls_pos = vec3(xChunkOffset, yPos, zChunkOffset);
    gl_Position = projection * view * vec4(os_pos , 1.0f);

    uint index = block1 & 0x3u;
    TexCoord = vec2(texCoords[index].x, 1.0 - texCoords[index].y) ;
    */
}