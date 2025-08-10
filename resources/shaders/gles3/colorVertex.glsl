#version 300 es

uniform mat4 mvp;
layout(location = 0) in vec3 vp;
uniform float pointSize;

void main() {
    gl_Position = mvp * vec4(vp, 1.0);
    gl_PointSize = pointSize;
}
