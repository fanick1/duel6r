#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
} ubo;

layout(location = 0) in vec3 vp;

void main() {
    gl_Position = ubo.mvp * vec4(vp, 1.0);
}
