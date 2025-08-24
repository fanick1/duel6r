#version 450

layout(binding = 1) uniform sampler2DArray textureUnit;

layout(binding = 2) uniform MaterialUniform {
    bool alphaTest;
    vec4 modulateColor;
    float globalTime;
} materialUbo;

layout(location = 0) in vec3 texCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 brightColor;

void main() {
    vec4 color = texture(textureUnit, texCoord);
    if (materialUbo.alphaTest && color.w < 1.0) {
        discard;
    }
    outColor = color * materialUbo.modulateColor;

    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 0.95) {
        brightColor = color.rgba * brightness;
    }
    else {
        brightColor = vec4(color.rgb, 0.0);
    }
}
