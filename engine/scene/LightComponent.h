// LightComponent.h - 灯光组件系统
#ifndef LIGHT_COMPONENT_H
#define LIGHT_COMPONENT_H

#include "Component.h"
#include <glm/glm.hpp>
#include <vector>

// 前向声明
class Shader;
class GameObject;

/**
 * @enum LightType
 * @brief 灯光类型
 */
enum class LightType {
    DIRECTIONAL,  // 方向光（如太阳光）
    POINT,        // 点光源
    SPOT          // 聚光灯
};

/**
 * @class LightComponent
 * @brief 灯光组件基类
 */
class LightComponent : public Component {
protected:
    LightType m_type;
    glm::vec3 m_color;
    float m_intensity;
    bool m_castShadows;

public:
    LightComponent(GameObject* owner, LightType type, 
                   const glm::vec3& color = glm::vec3(1.0f), 
                   float intensity = 1.0f);
    
    virtual ~LightComponent() = default;

    // ===== Getter/Setter =====
    
    LightType getType() const { return m_type; }
    
    const glm::vec3& getColor() const { return m_color; }
    void setColor(const glm::vec3& color) { m_color = color; }
    
    float getIntensity() const { return m_intensity; }
    void setIntensity(float intensity) { m_intensity = intensity; }
    
    bool getCastShadows() const { return m_castShadows; }
    void setCastShadows(bool cast) { m_castShadows = cast; }
    
    /**
     * @brief 应用灯光参数到着色器
     * @param shader 目标着色器
     * @param uniformPrefix 着色器中的uniform前缀（如 "lights[0]"）
     */
    virtual void applyToShader(Shader* shader, const std::string& uniformPrefix) const = 0;
    
    // Component 接口
    const char* getTypeName() const override { return "LightComponent"; }
};

/**
 * @class DirectionalLight
 * @brief 方向光组件（模拟太阳光等平行光）
 * 
 * 特点：
 * - 无位置，只有方向
 * - 影响整个场景
 * - 无衰减
 */
class DirectionalLight : public LightComponent {
private:
    glm::vec3 m_direction;

public:
    DirectionalLight(GameObject* owner, 
                     const glm::vec3& direction = glm::vec3(0.0f, -1.0f, 0.0f),
                     const glm::vec3& color = glm::vec3(1.0f),
                     float intensity = 1.0f);
    
    const glm::vec3& getDirection() const { return m_direction; }
    void setDirection(const glm::vec3& direction);
    
    /**
     * @brief 获取世界空间方向（考虑对象的旋转）
     */
    glm::vec3 getWorldDirection() const;
    
    void applyToShader(Shader* shader, const std::string& uniformPrefix) const override;
    
    const char* getTypeName() const override { return "DirectionalLight"; }
};

/**
 * @class PointLight
 * @brief 点光源组件（模拟灯泡等点光源）
 * 
 * 特点：
 * - 从一个点向四周发光
 * - 有距离衰减
 * - 影响范围有限
 */
class PointLight : public LightComponent {
private:
    float m_range;              // 影响范围
    float m_constantAtten;      // 常数衰减因子
    float m_linearAtten;        // 线性衰减因子
    float m_quadraticAtten;     // 二次衰减因子

public:
    PointLight(GameObject* owner,
               const glm::vec3& color = glm::vec3(1.0f),
               float intensity = 1.0f,
               float range = 10.0f);
    
    float getRange() const { return m_range; }
    void setRange(float range) { m_range = range; }
    
    void getAttenuation(float& constant, float& linear, float& quadratic) const {
        constant = m_constantAtten;
        linear = m_linearAtten;
        quadratic = m_quadraticAtten;
    }
    
    void setAttenuation(float constant, float linear, float quadratic) {
        m_constantAtten = constant;
        m_linearAtten = linear;
        m_quadraticAtten = quadratic;
    }
    
    /**
     * @brief 计算指定距离处的衰减系数
     * @param distance 距离
     * @return 衰减系数 (0-1)
     */
    float calculateAttenuation(float distance) const;
    
    void applyToShader(Shader* shader, const std::string& uniformPrefix) const override;
    
    const char* getTypeName() const override { return "PointLight"; }
};

/**
 * @class SpotLight
 * @brief 聚光灯组件（模拟手电筒等聚光灯）
 * 
 * 特点：
 * - 从一个点沿方向发出锥形光
 * - 有内锥角和外锥角
 * - 有距离衰减
 */
class SpotLight : public LightComponent {
private:
    glm::vec3 m_direction;
    float m_innerConeAngle;  // 内锥角（度）
    float m_outerConeAngle;  // 外锥角（度）
    float m_range;

public:
    SpotLight(GameObject* owner,
              const glm::vec3& direction = glm::vec3(0.0f, -1.0f, 0.0f),
              const glm::vec3& color = glm::vec3(1.0f),
              float intensity = 1.0f,
              float innerAngle = 12.5f,
              float outerAngle = 17.5f,
              float range = 15.0f);
    
    const glm::vec3& getDirection() const { return m_direction; }
    void setDirection(const glm::vec3& direction);
    
    /**
     * @brief 获取世界空间方向
     */
    glm::vec3 getWorldDirection() const;
    
    float getInnerConeAngle() const { return m_innerConeAngle; }
    void setInnerConeAngle(float angle) { m_innerConeAngle = angle; }
    
    float getOuterConeAngle() const { return m_outerConeAngle; }
    void setOuterConeAngle(float angle) { m_outerConeAngle = angle; }
    
    float getRange() const { return m_range; }
    void setRange(float range) { m_range = range; }
    
    void applyToShader(Shader* shader, const std::string& uniformPrefix) const override;
    
    const char* getTypeName() const override { return "SpotLight"; }
};

/**
 * @class LightManager
 * @brief 灯光管理器 - 管理场景中的所有灯光
 * 
 * 功能：
 * - 自动收集场景中的所有灯光组件
 * - 批量应用灯光参数到着色器
 * - 空间查询（查找影响特定区域的灯光）
 */
class LightManager {
private:
    std::vector<DirectionalLight*> m_directionalLights;
    std::vector<PointLight*> m_pointLights;
    std::vector<SpotLight*> m_spotLights;

public:
    LightManager() = default;
    ~LightManager() = default;

    /**
     * @brief 注册灯光组件
     * @param light 灯光组件指针
     */
    void registerLight(LightComponent* light);
    
    /**
     * @brief 注销灯光组件
     * @param light 灯光组件指针
     */
    void unregisterLight(LightComponent* light);
    
    /**
     * @brief 清空所有灯光
     */
    void clear();
    
    /**
     * @brief 更新着色器的所有灯光uniform
     * @param shader 目标着色器
     */
    void updateShaderUniforms(Shader* shader) const;
    
    /**
     * @brief 获取指定范围内的点光源
     * @param position 中心位置
     * @param radius 搜索半径
     * @return 点光源列表
     */
    std::vector<PointLight*> getPointLightsInRange(const glm::vec3& position, float radius) const;
    
    // ===== Getter =====
    
    const std::vector<DirectionalLight*>& getDirectionalLights() const { return m_directionalLights; }
    const std::vector<PointLight*>& getPointLights() const { return m_pointLights; }
    const std::vector<SpotLight*>& getSpotLights() const { return m_spotLights; }
    
    size_t getTotalLightCount() const {
        return m_directionalLights.size() + m_pointLights.size() + m_spotLights.size();
    }
};

#endif // LIGHT_COMPONENT_H
