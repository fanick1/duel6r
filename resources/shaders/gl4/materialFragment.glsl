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
  //  brightColor = color * modulateColor;
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
     if(brightness > 0.9)
         brightColor = vec4(color.rgb, 1.0);
     else
         brightColor = vec4(0.0, 0.0, 0.0, 1.0);

}