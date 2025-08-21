#version 450

layout(binding = 1) uniform ColorUniform {
    vec4 color;
} colorUbo;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = colorUbo.color;
}
