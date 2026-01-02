#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 uSlimeColor;
uniform vec3 uCameraPos;
uniform float uTime;

// 光源设置
const vec3 lightPos = vec3(10.0, 20.0, 10.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);

void main()
{
    // 标准化法线和视角方向
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(uCameraPos - FragPos);
    vec3 lightDir = normalize(lightPos - FragPos);
    
    // ===== 1. 环境光 =====
    float ambientStrength = 0.35;
    vec3 ambient = ambientStrength * lightColor;
    
    // ===== 2. 漫反射（Lambert）=====
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // ===== 3. 镜面反射（Blinn-Phong）=====
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), 64.0);
    float specularStrength = 0.6;
    vec3 specular = specularStrength * spec * lightColor;
    
    // ===== 4. Fresnel 效果（边缘高光）=====
    float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), 3.0);
    vec3 fresnelColor = vec3(0.9, 0.95, 1.0) * fresnel * 0.4;
    
    // ===== 5. 次表面散射近似（半透明感）=====
    float subsurface = pow(max(dot(-norm, lightDir), 0.0), 2.0);
    vec3 subsurfaceColor = uSlimeColor * subsurface * 0.5;
    
    // ===== 6. 边缘光（Rim Light）=====
    float rimDot = 1.0 - max(dot(viewDir, norm), 0.0);
    float rimLight = smoothstep(0.6, 1.0, rimDot);
    vec3 rimColor = uSlimeColor * 1.3 * rimLight * 0.25;
    
    // ===== 7. 动态果冻效果 =====
    float jelloPulse = sin(uTime * 2.0 + FragPos.y * 1.2) * 0.04 + 0.96;
    
    // ===== 组合所有光照 =====
    vec3 result = (ambient + diffuse) * uSlimeColor * jelloPulse
                  + specular 
                  + fresnelColor 
                  + subsurfaceColor 
                  + rimColor;
    
    // ===== 色调映射和伽马校正 =====
    // Reinhard tone mapping
    result = result / (result + vec3(1.0));
    // 伽马校正
    result = pow(result, vec3(1.0 / 2.2));
    
    // 半透明效果
    float alpha = 0.65;
    
    FragColor = vec4(result, alpha);
}