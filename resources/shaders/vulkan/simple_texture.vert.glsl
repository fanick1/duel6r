#version 450

// Push constant for MVP matrix
layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pc;

// Vertex inputs (MaterialVertex layout)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in float inTexIndex;
layout(location = 3) in uint inFlags;

// Outputs to fragment shader
layout(location = 0) out vec3 fragTexCoord;  // vec3(u, v, layerIndex)

void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
    fragTexCoord = vec3(inTexCoord, inTexIndex);
}
