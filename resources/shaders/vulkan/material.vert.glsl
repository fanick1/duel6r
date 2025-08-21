#version 450

const float waveHeight = 0.1;

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    float globalTime;
} ubo;

layout(location = 0) in vec3 vp;
layout(location = 1) in vec2 uvIn;
layout(location = 2) in float texIndexIn;
layout(location = 3) in uint flagsIn;

layout(location = 0) out vec3 uv;

vec3 waterWave(in vec3 position) {
    float displacement = sin(ubo.globalTime * 2.13 + 1.05 * position.x) * waveHeight;
    return vec3(position.x, position.y - waveHeight + displacement, position.z);
}

void main() {
    vec3 pos = flagsIn == 1 ? waterWave(vp) : vp;
    gl_Position = ubo.mvp * vec4(pos, 1.0);
    uv = vec3(uvIn, texIndexIn);
}
