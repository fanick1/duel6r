#version 450

// Shared by both stages, see VulkanBatchRenderer::flush
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 modulateColor;
    uint alphaTest;
    float time;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inTexCoord;  // u, v, texture layer
layout(location = 2) in vec4 inColor;
layout(location = 3) in uint inFlags;     // Duel6::Vertex::Flag

layout(location = 0) out vec3 fragTexCoord;
layout(location = 1) out vec4 fragColor;

const float waveHeight = 0.1;

vec3 waterWave(vec3 position) {
    float displacement = sin(pc.time * 2.13 + 1.05 * position.x) * waveHeight;
    return vec3(position.x, position.y - waveHeight + displacement, position.z);
}

void main() {
    vec3 position = inFlags == 1u ? waterWave(inPosition) : inPosition;
    gl_Position = pc.mvp * vec4(position, 1.0);
    gl_PointSize = 1.0;
    fragTexCoord = inTexCoord;
    fragColor = inColor;
}
