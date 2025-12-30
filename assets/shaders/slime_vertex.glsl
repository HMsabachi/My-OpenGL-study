#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uTime;  // 时间变量，用于动画效果

void main()
{
    // 计算波动效果 - 让顶点沿法线方向移动
    float wave1 = sin(aPos.y * 3.0 + uTime * 2.0) * 0.05;
    float wave2 = sin(aPos.x * 2.5 + uTime * 1.5) * 0.03;
    float wave3 = cos(aPos.z * 3.5 + uTime * 1.8) * 0.04;
    
    // 组合波动效果
    vec3 offset = aNormal * (wave1 + wave2 + wave3);
    vec3 animatedPos = aPos + offset;
    
    // 计算世界空间位置
    FragPos = vec3(uModel * vec4(animatedPos, 1.0));
    
    // 变换法线到世界空间
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    
    // 传递纹理坐标
    TexCoord = aTexCoord;
    
    // 最终位置
    gl_Position = uProjection * uView * vec4(FragPos, 1.0);
}
