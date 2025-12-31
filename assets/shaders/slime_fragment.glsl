#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 uSlimeColor;

void main()
{
    // 最简单的着色：纯色 + 简单光照
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0)); // 固定光源方向
    
    // 简单的漫反射光照
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = uSlimeColor * (0.5 + 0.5 * diff); // 保证至少有50%亮度
    
    // 完全不透明，亮绿色
    FragColor = vec4(diffuse, 1.0);
}
