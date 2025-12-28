#version 460 core
layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec3 aNormal;
layout (location = 1) in vec2 aTex;

out vec3 vPos;
out vec2 vTex;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    gl_Position =  uProjection * uView * uModel * vec4(aPos, 1.0);
    vPos = aPos;
    vTex = aTex;
}