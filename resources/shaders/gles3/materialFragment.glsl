#version 300 es

precision highp float;
uniform sampler2DArray textureUnit;
uniform bool alphaTest;
uniform vec4 modulateColor;
uniform vec4 effectColor;

in vec3 uv;
out vec4 result;

void main() {
    vec4 color = texture(textureUnit, uv);
    if (alphaTest && color.w < 1.0) {
        discard;
    }
    if(effectColor.r != 0.0 || effectColor.g != 0.0 || effectColor.b != 0.0){
        result = vec4(effectColor.r, effectColor.g, effectColor.b, color.w * modulateColor.w);
    } else {
        result = color * modulateColor;
    }
}