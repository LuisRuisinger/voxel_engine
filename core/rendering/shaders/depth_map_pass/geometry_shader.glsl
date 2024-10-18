#version 410 core

layout(triangles, invocations = 4) in;
layout(triangle_strip, max_vertices = 3) out;

layout (std140) uniform LSMatrices {
    mat4 ls_matrices[4];
};

void main() {
    for (int i = 0; i < 3; ++i) {
        gl_Position = ls_matrices[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }

    EndPrimitive();
}