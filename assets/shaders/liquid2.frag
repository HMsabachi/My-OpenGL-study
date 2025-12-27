#version 460 core
out vec4 FragColor;

/* ===== Shadertoy -> OpenGL uniforms =====
   iChannel0   -> sampler2D uTex
   iResolution -> vec2 uResolution (像素尺寸)
   iMouse.xy   -> vec2 uMouse      (像素坐标, (0,0) 表示未激活)
   iFrame      -> int  uFrame      (帧计数, >=0)
*/
uniform sampler2D uTex;
uniform vec2      uResolution;
uniform float     uCircleSize;
uniform vec2      uMouse;
uniform int       uFrame;

/* ===== 参数 ===== */
const float AIR_IOR               = 1.0003;
const float GLASS_IOR             = 1.75;
const float REFRACT_RT_DISTANCE   = 250.0;
const float REFLECT_RT_DISTANCE   = 150.0;
float CIRCLE_SIZE           = uCircleSize != 0.0? uCircleSize : 300.0;
const float EDGE_FRACTION         = 0.7;
const float EDGE_POWER            = 4.0;
const float NORMAL_JITER          = 0.02;

#define CHROMATIC_ABBERATION_STRENGTH 0

#define BLUR_LOD_BIAS 0.1
// 若使用注释里的壁纸，可加大模糊：
// #define BLUR_LOD_BIAS 2.5

#define PI    3.14159265358979323846
#define RCP_PI (1.0/PI)

/* ------------ IQ: TextureNice------------ */
vec4 textureNice(sampler2D sam, vec2 uv, int level)
{
    float textureResolution = float(textureSize(sam, level).x);
    uv = uv * textureResolution + 0.5;
    vec2 iuv = floor(uv);
    vec2 fuv = fract(uv);
    uv = iuv + fuv * fuv * (3.0 - 2.0 * fuv);
    uv = (uv - 0.5) / textureResolution;
    return pow(textureLod(sam, uv, float(level)), vec4(2.2));
}

vec4 textureNiceTrilinear(sampler2D sam, vec2 uv, float lod)
{
    float interpo = fract(lod);
    int   floorLod = int(floor(lod));
    vec4  base   = textureNice(sam, uv, floorLod);
    vec4  higher = textureNice(sam, uv, floorLod + 1);
    return mix(base, higher, interpo);
}

/* -------------------- 工具函数 -------------------- */
float rand_IGN(vec2 v, uint frame) {
    frame = frame % 64u;
    v = v + 5.588238 * float(frame);
    return fract(52.9829189 * fract(0.06711056 * v.x + 0.00583715 * v.y));
}

float pow2(float x){ return x * x; }
float pow5(float x){ float x2 = x * x; return x2 * x2 * x; }

#define rcp(x)      (1.0 / (x))
#define saturate(x) clamp(x, 0.0, 1.0)

float linearStep(float edge0, float edge1, float x) {
    return saturate((x - edge0) / (edge1 - edge0));
}

/* -------------------- Fresnel/BSDF -------------------- */
float fresnel_iorToF0(float ior) {
    return pow2((ior - AIR_IOR) / (ior + AIR_IOR));
}
float fresnel_schlick(float cosTheta, float f0) {
    return f0 + (1.0 - f0) * pow5(1.0 - cosTheta);
}

float _bsdf_g_Smith_Schlick_denom(float cosTheta, float k) {
    return cosTheta * (1.0 - k) + k;
}
float bsdf_ggx(float roughness, float NDotL, float NDotV, float NDotH) {
    if (NDotL <= 0.0) return 0.0;
    float NDotH2 = pow2(NDotH);
    float a2 = pow2(roughness);
    float d = a2 / (PI * pow2(NDotH2 * (a2 - 1.0) + 1.0));
    float k = roughness * 0.5;
    float v = rcp(_bsdf_g_Smith_Schlick_denom(NDotL, k) *
                  _bsdf_g_Smith_Schlick_denom(saturate(NDotV), k));
    return NDotL * d * v;
}

/* -------------------- 核心实现 -------------------- */
const vec3 INCIDENT_VECTOR = vec3(0.0, 0.0, 1.0);

float glassIorCA(float wavelength) {
    const float abberation = float(CHROMATIC_ABBERATION_STRENGTH) * 0.1;
    // 波长越短 IOR 越高
    float glassIor = mix(GLASS_IOR + abberation, GLASS_IOR - abberation,
                         1.0 - pow(1.0 - linearStep(450.0, 650.0, wavelength), 4.0));
    return glassIor;
}

vec3 sampleRefraction(vec2 fragCoord, float sdfValue, vec3 normal, float glassIor) {
    vec3 refractVector = refract(INCIDENT_VECTOR, normal, AIR_IOR / glassIor);
    refractVector /= abs(refractVector.z / (REFRACT_RT_DISTANCE));
    vec2 refractedUV = (fragCoord + refractVector.xy) / uResolution;
    vec3 refractedColor = textureNiceTrilinear(uTex, refractedUV, sdfValue * 2.0 + BLUR_LOD_BIAS).rgb;
    return refractedColor;
}

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 uv = fragCoord / uResolution;

    vec3 bg    = textureNice(uTex, uv, 0).rgb;
    vec3 color = bg;

    float randV     = rand_IGN(fragCoord, uint(uFrame));
    float randAngle = randV * PI * 2.0;

    vec2 circleCenter = (uMouse == vec2(0.0)) ? (uResolution * 0.5) : uMouse;
    float circleDist  = distance(fragCoord, circleCenter);

    float sdfValue = pow(linearStep(CIRCLE_SIZE * EDGE_FRACTION, CIRCLE_SIZE, circleDist), EDGE_POWER);

    vec3 normal = mix(
        normalize(vec3(sin(randAngle), cos(randAngle), -rcp(NORMAL_JITER))),
        vec3(normalize(fragCoord - circleCenter), 0.0),
        sdfValue
    );
    normal = normalize(normal);

    // 折射（含色差）
    vec3 refractedColor;
#if CHROMATIC_ABBERATION_STRENGTH
    refractedColor  = sampleRefraction(fragCoord, sdfValue, normal, glassIorCA(611.4)) * vec3(1.0, 0.0, 0.0);
    refractedColor += sampleRefraction(fragCoord, sdfValue, normal, glassIorCA(570.5)) * vec3(1.0, 1.0, 0.0);
    refractedColor += sampleRefraction(fragCoord, sdfValue, normal, glassIorCA(549.1)) * vec3(0.0, 1.0, 0.0);
    refractedColor += sampleRefraction(fragCoord, sdfValue, normal, glassIorCA(491.4)) * vec3(0.0, 1.0, 1.0);
    refractedColor += sampleRefraction(fragCoord, sdfValue, normal, glassIorCA(464.2)) * vec3(0.0, 0.0, 1.0);
    refractedColor += sampleRefraction(fragCoord, sdfValue, normal, glassIorCA(374.0)) * vec3(1.0, 0.0, 1.0);
    refractedColor /= 3.0;
#else
    refractedColor = sampleRefraction(fragCoord, sdfValue, normal, GLASS_IOR);
#endif

    // 反射 + GGX 高光
    const vec3 V = vec3(0.0, 0.0, -1.0);
    float NDotV = saturate(dot(V, normal));
    float fresnelV = fresnel_schlick(NDotV, fresnel_iorToF0(GLASS_IOR));

    vec3 reflectVector = reflect(INCIDENT_VECTOR, normal);
    vec3 L = reflectVector;
    vec3 H = normalize(L + V);

    reflectVector /= abs(reflectVector.z / (REFLECT_RT_DISTANCE));
    vec2 reflectedUV = (fragCoord + reflectVector.xy) / uResolution;
    vec3 reflectedColor = textureNiceTrilinear(uTex, reflectedUV, 2.5 + BLUR_LOD_BIAS).rgb;

    float NDotL = dot(normal, L);
    float NDotH = dot(normal, H);
    float ggx   = bsdf_ggx(0.5, NDotL, NDotV, NDotH);
    reflectedColor *= ggx;

    vec3 glassColor = mix(refractedColor, reflectedColor, fresnelV);

    // 圆形范围内混合玻璃效果
    color = mix(color, glassColor, smoothstep(CIRCLE_SIZE, CIRCLE_SIZE - 2.0, circleDist));

    // 伽马还原
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
