#version 450

// Texture sampler (2D array texture)
layout(set = 0, binding = 0) uniform sampler2DArray textureUnit;

// Push constants for material parameters (offset after mat4 MVP = 64 bytes)
layout(push_constant) uniform PushConstants {
    layout(offset = 64) vec4 modulateColor;
    layout(offset = 80) uint alphaTest;
} material;

// Input from vertex shader
layout(location = 0) in vec3 fragTexCoord;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(textureUnit, fragTexCoord);
    
    // Alpha test: discard fragments with alpha < 1.0 when masked
    if (material.alphaTest != 0u && texColor.a < 1.0) {
        discard;
    }
    
    outColor = texColor * material.modulateColor;
}
