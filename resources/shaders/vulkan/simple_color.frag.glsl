#version 450

// Simple color shader - just passes through the interpolated color

layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = fragColor;
}
