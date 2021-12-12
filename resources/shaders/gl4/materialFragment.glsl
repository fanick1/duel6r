#version 430

layout(binding = 0) uniform sampler2DArray textureUnit;

uniform bool alphaTest;
uniform vec4 modulateColor;
uniform float globalTime;

in vec3 uv;
layout (location = 0) out vec4 result;
layout (location = 1) out vec4 brightColor;

void main() {
    vec4 color = texture(textureUnit, uv);
    if (alphaTest && color.w < 1.0) {
        discard;
    }
    result = color * modulateColor;

    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 0.95) {
        //        brightColor = color.rgba;
        brightColor = color.rgba * brightness;//vec4(color.rgba * brightness, 1.0);
    }
    else {
        brightColor = vec4(color.rgb, 0.0);
    }
}