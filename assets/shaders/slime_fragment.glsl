#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 uSlimeColor = vec3(0.5f, 0.7f, 1.0f);
uniform vec3 uCameraPos;
uniform float uTime;

// 光源设置
const vec3 lightPos = vec3(10.0, 20.0, 10.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);

// 二次元风格参数
const float toonLevels = 3.0;  // 明暗分层数量
const float outlineThreshold = 0.2;  // 描边阈值
const vec3 outlineColor = vec3(0.1, 0.1, 0.15);  // 描边颜色

void main()
{
    // 标准化法线和视角方向
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(uCameraPos - FragPos);
    
    // === 1. 边缘检测（描边效果）===
    float rimDot = 1.0 - max(dot(viewDir, norm), 0.0);
    float rimIntensity = rimDot * rimDot;
    
    // 如果在边缘，渲染描边
    if (rimIntensity > outlineThreshold) {
        FragColor = vec4(outlineColor, 1.0);
        return;
    }
    
    // === 2. 卡通式光照（阶梯式明暗）===
    vec3 lightDir = normalize(lightPos - FragPos);
    float NdotL = max(dot(norm, lightDir), 0.0);
    
    // 将光照强度量化为几个离散级别
    float toonDiff = floor(NdotL * toonLevels) / toonLevels;
    
    // 环境光 + 漫反射
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * uSlimeColor;
    vec3 diffuse = toonDiff * uSlimeColor;
    
    // === 3. 二次元高光（圆形高光点）===
    vec3 halfDir = normalize(lightDir + viewDir);
    float NdotH = max(dot(norm, halfDir), 0.0);
    
    // 创建锐利的高光边界
    float specular = pow(NdotH, 64.0);
    specular = step(0.5, specular);  // 二值化高光
    
    // 高光颜色（偏白偏亮）
    vec3 specularColor = vec3(1.0, 1.0, 1.0) * specular * 0.8;
    
    // === 4. Rim Light（边缘光，增强轮廓）===
    float rimLight = smoothstep(0.6, 1.0, rimDot);
    vec3 rimColor = uSlimeColor * 1.5 * rimLight * 0.3;
    
    // === 5. 史莱姆特有的果冻感（可选的动态效果）===
    float jelloPulse = sin(uTime * 3.0 + FragPos.y * 2.0) * 0.05 + 0.95;
    
    // === 最终组合 ===
    vec3 finalColor = (ambient + diffuse) * jelloPulse + specularColor + rimColor;
    
    // 增加饱和度，让颜色更鲜艳
    finalColor = mix(vec3(dot(finalColor, vec3(0.299, 0.587, 0.114))), finalColor, 1.3);
    
    FragColor = vec4(finalColor, 1.0);
}