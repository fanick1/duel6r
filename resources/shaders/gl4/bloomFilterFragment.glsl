#version 430

layout(binding = 0) uniform sampler2DArray textureUnit;

uniform bool alphaTest;
uniform vec4 modulateColor;
uniform float globalTime;
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

in vec3 uv;
layout (location = 0) out vec4 result;

void main() {
    vec3 tex_offset = 1.0 / textureSize(textureUnit, 0); // gets size of single texel
    float blur = 1.0;
    vec4 color = texture(textureUnit, uv);
    if (alphaTest && color.w < 1.0) {
        discard;
    }
    result = color * modulateColor;
    for(int i = 0; i < 5; ++i) {
        result += texture(textureUnit, uv + vec3(tex_offset.x * i * blur, 0.0, 0.0)).rgba * weight[i];
        result += texture(textureUnit, uv - vec3(tex_offset.x * i * blur, 0.0, 0.0)).rgba * weight[i];
    }
    for(int i = 0; i < 5; ++i) {
        result += texture(textureUnit, uv + vec3(0, tex_offset.y * i * blur, 0.0)).rgba * weight[i];
        result += texture(textureUnit, uv - vec3(0, tex_offset.y * i * blur, 0.0)).rgba * weight[i];
    }
}