#version 460 core

in vec3 vPos;
in vec3 vColor;
in vec2 vTex;

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
    FragColor = mix(texture(texture1, vTex), texture(texture2, vTex), 0.2);
} 