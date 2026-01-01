#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    
    // 变换法线到世界空间
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    
    gl_Position = uProjection * uView * worldPos;
}
