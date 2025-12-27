#version 460 core

in vec3 aPos;     // 位置
in vec3 aColor;   // 颜色
in vec2 aUV;      // UV

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

out vec2 vUV;      // 备用
out vec3 vColor;   // 备用

void main()
{
    vUV = aUV;
    vColor = aColor;
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
