#version 450

layout(set = 0, binding = 0) uniform sampler2D depthTexture;

layout(location = 0) in vec2 fragTexCoord;

void main() {
    gl_FragDepth = texture(depthTexture, fragTexCoord).r;
}
