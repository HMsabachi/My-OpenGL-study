#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;

void main()
{
    FragColor = vec4(0.8,0.2,0.0,1.0);
}
