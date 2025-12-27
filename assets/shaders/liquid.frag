#version 460 core
out vec4 FragColor;

uniform sampler2D uTex;        // 背景
uniform vec2      uResolution; // 像素分辨率
uniform vec2      uMouse;      // 像素坐标；(0,0) 则用中心
uniform int       uFrame;      // 帧计数（做随机用）

/* ===== 全部参数写死在这里 ===== */
const float AIR_IOR             = 1.0003;
const float GLASS_IOR           = 1.65;    // 折射率
const float REFRACT_RT_DISTANCE = 350.0;   // 折射“射线长度”
const float REFLECT_RT_DISTANCE = 350.0;   // 反射“射线长度”
const float EDGE_FRACTION       = 0.70;    // 厚边占比
const float EDGE_POWER          = 4.0;     // 厚边曲线幂
const float NORMAL_JITTER       = 0.02;    // 法线抖动
const float BLUR_LOD_BIAS       = 0.8;     // 模糊力度（要有 mipmap）
const int   CHROM_AB            = 1;       // 1 开启色差，0 关闭

// 圆角矩形形状（像素）
const vec2  RECT_HALF_SIZE      = vec2(220.0, 140.0); // 半宽、半高
const float CORNER_R            = 40.0;               // 圆角半径
const float EDGE_FEATHER        = 1.5;                // 边缘羽化像素

/* ---------- 工具 ---------- */
float pow2(float x){ return x*x; }
float pow5(float x){ float x2=x*x; return x2*x2*x; }
#define saturate(x) clamp(x,0.0,1.0)
float linearStep(float a,float b,float x){ return saturate((x-a)/(b-a)); }

float sdfRoundRect(vec2 p, vec2 center, vec2 size, float r)
{
    vec2 q = abs(p - center) - size + vec2(r);
    return length(max(q,0.0)) + min(max(q.x,q.y),0.0) - r;
}

const float PI = 3.14159265358979323846;
const vec3 INCIDENT = vec3(0.0,0.0,1.0);

float fresnel_iorToF0(float ior){ return pow2((ior - AIR_IOR)/(ior + AIR_IOR)); }
float fresnel_schlick(float cosTheta, float f0){ return f0 + (1.0 - f0)*pow5(1.0 - cosTheta); }
float _gSmithDenom(float c,float k){ return c*(1.0-k)+k; }
float bsdf_ggx(float rough,float NdotL,float NdotV,float NdotH){
    if(NdotL<=0.0) return 0.0;
    float a2 = rough*rough;
    float d = a2 / (PI * pow2(pow2(NdotH)*(a2-1.0)+1.0));
    float k = rough*0.5;
    float v = 1.0/(_gSmithDenom(NdotL,k)*_gSmithDenom(saturate(NdotV),k));
    return NdotL * d * v;
}

vec3 sampleRefraction(vec2 fragCoord, float sdf01, vec3 normal, float glassIOR)
{
    vec3 r = refract(INCIDENT, normal, AIR_IOR/glassIOR);
    r /= abs(r.z/(REFRACT_RT_DISTANCE));
    vec2 uv = (fragCoord + r.xy) / uResolution;
    return textureLod(uTex, uv, sdf01*2.0 + BLUR_LOD_BIAS).rgb;
}

float rand(vec2 v, uint frame){
    frame %= 64u;
    v = v + 5.588238*float(frame);
    return fract(52.9829189 * fract(0.06711056*v.x + 0.00583715*v.y));
}

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 uv = fragCoord / uResolution;

    vec2 center = (uMouse == vec2(0.0)) ? (uResolution*0.5) : uMouse;

    // 圆角矩形 SDF
    float sd = sdfRoundRect(fragCoord, center, RECT_HALF_SIZE, CORNER_R);

    // 厚边权重（0~1）：越靠近边缘越强
    float sizePx   = max(RECT_HALF_SIZE.x, RECT_HALF_SIZE.y);
    float edgeInPx = -EDGE_FRACTION * 0.02 * sizePx;         // 经验映射
    float sdf01    = pow(linearStep(edgeInPx, 0.0, -sd), EDGE_POWER);

    // 边缘 AA
    float mask = smoothstep(EDGE_FEATHER, -EDGE_FEATHER, sd);

    // 法线（屏幕梯度 + 抖动 + “抬高”）
    float dx = dFdx(sd), dy = dFdy(sd);
    float ncos = saturate((0.5*sizePx*sdf01 + sd) / max(1.0, 0.5*sizePx*sdf01 + 1e-3));
    float nsin = sqrt(max(1.0 - ncos*ncos, 0.0));
    float a = rand(fragCoord, uint(uFrame)) * 6.2831853;
    vec3 normal = normalize(vec3(dx*ncos + sin(a)*NORMAL_JITTER,
                                 dy*ncos + cos(a)*NORMAL_JITTER,
                                 nsin));

    vec3 bg = texture(uTex, uv).rgb;

    // 折射（可选色差）
    vec3 refrC = vec3(0.0);
    if(CHROM_AB==1){
        refrC += sampleRefraction(fragCoord, sdf01, normal, 1.58) * vec3(1,0,0);
        refrC += sampleRefraction(fragCoord, sdf01, normal, 1.52) * vec3(1,1,0);
        refrC += sampleRefraction(fragCoord, sdf01, normal, 1.49) * vec3(0,1,0);
        refrC += sampleRefraction(fragCoord, sdf01, normal, 1.47) * vec3(0,1,1);
        refrC += sampleRefraction(fragCoord, sdf01, normal, 1.45) * vec3(0,0,1);
        refrC += sampleRefraction(fragCoord, sdf01, normal, 1.56) * vec3(1,0,1);
        refrC /= 3.0;
    }else{
        refrC = sampleRefraction(fragCoord, sdf01, normal, GLASS_IOR);
    }

    // 反射 + GGX
    const vec3 V = vec3(0.0,0.0,-1.0);
    float NdotV = saturate(dot(V, normal));
    float F = fresnel_schlick(NdotV, fresnel_iorToF0(GLASS_IOR));

    vec3 r = reflect(INCIDENT, normal);
    vec3 L = r;
    vec3 H = normalize(L + V);
    r /= abs(r.z/(REFLECT_RT_DISTANCE));
    vec2 uvR = (fragCoord + r.xy) / uResolution;
    vec3 refl = textureLod(uTex, uvR, 2.5 + BLUR_LOD_BIAS).rgb;

    float NdotL = dot(normal, L);
    float NdotH = dot(normal, H);
    refl *= bsdf_ggx(0.5, NdotL, NdotV, NdotH);

    vec3 glass = mix(refrC, refl, F);

    vec3 color = mix(bg, glass, mask);


    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
