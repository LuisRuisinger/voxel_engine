// Daniel Shervheim, 2019
// danielshervheim.com
// modifications by Luis S. Ruisinger, 2024

#version 410 core

layout (location = 0) in vec3 position;

// uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 camera;

out vec3 FragPos;

void main()  {

    // Center the skysphere over the camera position.
    vec4 posWS = vec4(camera + position.xzy, 1.0F);
    vec4 posVS = view * posWS;
    vec4 posCS = projection * posVS;

    FragPos = posWS.xyz;

    // Set z to w, in other words as far as possible.
    // This makes the skysphere render behind everything else.
    gl_Position = posCS.xyww;
}
