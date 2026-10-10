#version 450

// Copies an offscreen depth image into the depth buffer (color writes are off in the pipeline)
layout(set = 0, binding = 0) uniform sampler2DArray depthTexture;

layout(location = 0) in vec3 fragTexCoord;

void main() {
    gl_FragDepth = texture(depthTexture, fragTexCoord).r;
}
