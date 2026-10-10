#version 450

layout(set = 0, binding = 0) uniform sampler2DArray textureUnit;

layout(push_constant) uniform PushConstants {
    layout(offset = 64) vec4 modulateColor;
    layout(offset = 80) uint alphaTest;
} pc;

layout(location = 0) in vec3 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 color = texture(textureUnit, fragTexCoord);
    if (pc.alphaTest != 0u && color.a < 1.0) {
        discard;
    }
    // Untextured geometry samples a 1x1 white texture, so only fragColor remains
    outColor = color * fragColor * pc.modulateColor;
}
