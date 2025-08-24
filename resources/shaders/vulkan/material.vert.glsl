#version 450

const float waveHeight = 0.1;

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    float globalTime;
} ubo;

layout(location = 0) in vec3 vp;
layout(location = 1) in vec2 uv;
layout(location = 2) in float texIndex;
layout(location = 3) in uint flags;

layout(location = 0) out vec3 texCoord;

vec3 waterWave(in vec3 position) {
    float displacement = sin(ubo.globalTime * 2.13 + 1.05 * position.x) * waveHeight;
    return vec3(position.x, position.y - waveHeight + displacement, position.z);
}

void main() {
    vec3 pos = flags == 1 ? waterWave(vp) : vp;
    gl_Position = ubo.mvp * vec4(pos, 1.0);
    texCoord = vec3(uv, texIndex);
}
