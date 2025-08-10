#version 300 es

precision highp float;
uniform vec4 color;
uniform float pointSize;
uniform int isPoint;
out vec4 result;
void main() {
    if(isPoint == 1){
        vec2 coord = gl_PointCoord - vec2(0.5);
        if(length(coord) > 0.5) discard;
    }
    result = color;
}