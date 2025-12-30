#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 uCameraPos;      // 相机位置
uniform float uTime;          // 时间变量
uniform vec3 uSlimeColor;     // 史莱姆基础颜色

void main()
{
    // 归一化法线
    vec3 norm = normalize(Normal);
    
    // 计算视线方向
    vec3 viewDir = normalize(uCameraPos - FragPos);
    
    // 菲涅尔效果 - 边缘更亮，模拟液体的光泽
    float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), 3.0);
    
    // 动态颜色波动 - 让颜色随时间和位置变化
    float colorShift = sin(FragPos.y * 2.0 + uTime) * 0.5 + 0.5;
    vec3 dynamicColor = mix(uSlimeColor * 0.7, uSlimeColor * 1.3, colorShift);
    
    // 添加内部光晕效果
    vec3 innerGlow = uSlimeColor * 1.5 * fresnel;
    
    // 混合颜色
    vec3 finalColor = dynamicColor + innerGlow;
    
    // 半透明度 - 边缘更透明
    float alpha = 0.7 + fresnel * 0.3;
    
    FragColor = vec4(finalColor, alpha);
}
