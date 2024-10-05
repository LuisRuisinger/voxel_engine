#version 330 core

layout(location = 0) in uint high;
layout(location = 1) in uint low;

uniform vec2 worldbase;
uniform mat4 view;
uniform mat4 projection;
uniform uint render_radius;

out vec3 FragPos;
out vec3 FragNormal;
out vec3 FragTexture;

const vec3 normals[6] = vec3[](
    vec3(-1.0, 0.0, 0.0),
    vec3( 1.0, 0.0, 0.0),
    vec3(0.0, -1.0, 0.0),
    vec3(0.0,  1.0, 0.0),
    vec3(0.0, 0.0, -1.0),
    vec3(0.0, 0.0,  1.0)
);

// decompress world space position
vec3 world_space_chunk_pos() {
    int chunk_index = int(high >> 20U);
    int render_diameter = int(render_radius * 2U);

    float x = float((chunk_index % render_diameter) - int(render_radius));
    float y = float((high >> 16) & 0xFU);
    float z = float((chunk_index / render_diameter) - int(render_radius));

    return 32.0F * vec3(x, y, z);
}

// decompress object space position
vec3 object_space_object_pos(float scale) {
    vec3 compressed_object_pos = vec3(
        float(low >> 29U),
        float((low >> 26U) & 0x7U),
        float((low >> 23U) & 0x7U)
    );

    return compressed_object_pos * 0.25F * scale;
}

// decompress object space uv coordinates
vec3 object_space_texture_pos(float scale) {
    vec3 compressed_texture_pos = vec3(
        float((low >> 21U) & 0x3U) * 0.5F * scale,
        float((low >> 19U) & 0x3U) * 0.5F * scale,
        float((high & 0xFFU) * 4.0F + ((high >> 11) & 0x3U))
    );

    return compressed_texture_pos;
}

mat4 model_matrix(vec4 obj_space_vertex, vec4 model_space_vertex) {
    return outerProduct(model_space_vertex, obj_space_vertex) *
           inverse(outerProduct(obj_space_vertex, obj_space_vertex));
}

// decompress normals
vec3 normal() {
    return normals[(high >> 13) & 0x7U];
}

void main() {

    // unpacking
    float x_chunk_space  = float((low >> 13U) & 0x1FU);
    float y_chunk_space  = float((low >>  8U) & 0x1FU);
    float z_chunk_space  = float((low >>  3U) & 0x1FU);
    float scale          = float(1 << (low & 0x7U));

    vec3 obj_space_position = object_space_object_pos(scale);
    vec3 model_trans = vec3(x_chunk_space, y_chunk_space, z_chunk_space) +
                       world_space_chunk_pos() -
                       vec3(0.0F, 128.0F, 0.0F) +
                       vec3(worldbase.x, 0.0F, worldbase.y);
    vec3 position = obj_space_position + model_trans;

    if (scale == 1) {
        position += vec3(0.5F);
    }
    else {
        position -= vec3(0.5F) * (scale - 1);
    }

    FragPos = position;
    FragNormal =  normal();
    FragTexture = object_space_texture_pos(scale);
    gl_Position = projection * view * vec4(position, 1.0F);
}

