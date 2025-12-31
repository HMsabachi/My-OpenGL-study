#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

// 实例化属性 - 每个粒子的模型矩阵 (mat4 占用 4 个 location)
layout (location = 3) in vec4 aInstanceMatrix0;
layout (location = 4) in vec4 aInstanceMatrix1;
layout (location = 5) in vec4 aInstanceMatrix2;
layout (location = 6) in vec4 aInstanceMatrix3;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    // 构建实例化矩阵
    mat4 instanceMatrix = mat4(
        aInstanceMatrix0,
        aInstanceMatrix1,
        aInstanceMatrix2,
        aInstanceMatrix3
    );
    
    // 简单的变换，没有任何动画效果
    vec4 worldPos = instanceMatrix * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    
    // 变换法线
    Normal = mat3(transpose(inverse(instanceMatrix))) * aNormal;
    
    // 最终位置
    gl_Position = uProjection * uView * worldPos;
}
