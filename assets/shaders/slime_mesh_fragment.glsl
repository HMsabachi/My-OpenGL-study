#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 uSlimeColor;
uniform vec3 uCameraPos;
uniform float uTime;

// 光源位置
const vec3 lightPos = vec3(10.0, 20.0, 10.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);

void main()
{
    // 标准化法线
    vec3 norm = normalize(Normal);
    
    // 光照方向
    vec3 lightDir = normalize(lightPos - FragPos);
    
    // 环境光
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // 漫反射
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 镜面反射（半透明果冻效果）
    float specularStrength = 0.8;
    vec3 viewDir = normalize(uCameraPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Fresnel 效果（边缘高光）
    float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), 3.0);
    vec3 fresnelColor = vec3(0.8, 1.0, 0.9) * fresnel * 0.5;
    
    // 次表面散射近似（让史莱姆看起来半透明）
    float subsurface = pow(max(dot(-norm, lightDir), 0.0), 2.0);
    vec3 subsurfaceColor = uSlimeColor * subsurface * 0.4;
    
    // 组合所有光照
    vec3 result = (ambient + diffuse + subsurfaceColor) * uSlimeColor + specular + fresnelColor;
    
    // 添加一点动态变化（可选）
    float pulse = sin(uTime * 2.0) * 0.05 + 0.95;
    result *= pulse;
    
    // 半透明效果（史莱姆应该有一定透明度）
    float alpha = 0.85;
    
    FragColor = vec4(result, alpha);
}
