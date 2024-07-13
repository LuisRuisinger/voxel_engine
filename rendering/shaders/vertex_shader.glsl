#version 330 core

layout(location = 0) in uint high;
layout(location = 1) in uint low;

uniform vec2 worldbase;
uniform mat4 view;
uniform mat4 projection;
uniform int  render_radius;

out vec2 TexCoord;

vec3 fromIndex(int chunkIdx, uint segmentIdx) {
    int render_diameter = render_radius * 2;

    float x = float((chunkIdx % render_diameter) - render_radius);
    float y = float(segmentIdx);
    float z = float((chunkIdx / render_diameter) - render_radius);

    return 32.0F * vec3(x, y, z);
}

void main()  {
    float xObjectSpace = float( low >> 29U);
    float yObjectSpace = float((low >> 26U) & 0x7U);
    float zObjectSpace = float((low >> 23U) & 0x7U);

    float xChunkSpace  = float((low >> 13U) & 0x1FU);
    float yChunkSpace  = float((low >>  8U) & 0x1FU);
    float zChunkSpace  = float((low >>  3U) & 0x1FU);
    float scale        = float(1 << (low & 0x7U));

    float u            = float((low >> 21U) &  0x3U);
    float v            = float((low >> 19U) &  0x3U);

    uint chunkIdx  =  high >> 20;
    uint segmentdx = (high >> 16) & 0xFU;

    vec3 position = (vec3(xObjectSpace, yObjectSpace, zObjectSpace) * 0.25F * scale) +
                    vec3(xChunkSpace, yChunkSpace, zChunkSpace) +
                    vec3(-0.5) * (scale) +

                    // error correction
                    vec3(-0.5) * float(bool(scale > 1)) +
                    fromIndex(int(chunkIdx), segmentdx) - vec3(0.0F, 128.0F, 0.0F) +
                    vec3(worldbase.x, 0.0F, worldbase.y);

    vec2 tex = vec2(u, v) * 0.5F;

    gl_Position = projection * view * vec4(position, 1.0F);
    TexCoord = vec2(tex.x, tex.y - 1.0F);
}