// Transform.h - 变换组件
#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * @class Transform
 * @brief 变换组件 - 表示对象的位置、旋转和缩放
 * 
 * 每个 GameObject 都必须有一个 Transform 组件
 * 支持本地坐标和世界坐标的转换
 */
class Transform : public Component {
private:
    // 本地变换
    glm::vec3 m_localPosition;
    glm::quat m_localRotation;
    glm::vec3 m_localScale;
    
    // 缓存的变换矩阵
    mutable glm::mat4 m_localToWorldMatrix;
    mutable glm::mat4 m_worldToLocalMatrix;
    mutable bool m_matrixDirty;  // 矩阵是否需要重新计算

public:
    // 辅助方法
    void markDirty();
    void updateMatrices() const;

public:
    /**
     * @brief 构造函数
     * @param owner 拥有此组件的游戏对象
     */
    Transform(GameObject* owner);
    
    ~Transform() override = default;

    // ===== 本地变换 =====
    
    const glm::vec3& getLocalPosition() const { return m_localPosition; }
    void setLocalPosition(const glm::vec3& position);
    
    const glm::quat& getLocalRotation() const { return m_localRotation; }
    void setLocalRotation(const glm::quat& rotation);
    
    /**
     * @brief 设置本地旋转（欧拉角）
     * @param eulerAngles 欧拉角（度）
     */
    void setLocalRotationEuler(const glm::vec3& eulerAngles);
    
    /**
     * @brief 获取本地旋转（欧拉角）
     * @return 欧拉角（度）
     */
    glm::vec3 getLocalRotationEuler() const;
    
    const glm::vec3& getLocalScale() const { return m_localScale; }
    void setLocalScale(const glm::vec3& scale);

    // ===== 世界变换 =====
    
    /**
     * @brief 获取世界坐标位置
     */
    glm::vec3 getWorldPosition() const;
    
    /**
     * @brief 设置世界坐标位置
     */
    void setWorldPosition(const glm::vec3& position);
    
    /**
     * @brief 获取世界坐标旋转
     */
    glm::quat getWorldRotation() const;
    
    /**
     * @brief 设置世界坐标旋转
     */
    void setWorldRotation(const glm::quat& rotation);
    
    /**
     * @brief 获取世界坐标缩放
     */
    glm::vec3 getWorldScale() const;

    // ===== 变换矩阵 =====
    
    /**
     * @brief 获取本地到世界变换矩阵
     */
    const glm::mat4& getLocalToWorldMatrix() const;
    
    /**
     * @brief 获取世界到本地变换矩阵
     */
    const glm::mat4& getWorldToLocalMatrix() const;
    
    /**
     * @brief 获取本地变换矩阵（不考虑父对象）
     */
    glm::mat4 getLocalMatrix() const;

    // ===== 方向向量 =====
    
    /**
     * @brief 获取前方向（本地 Z 轴）
     */
    glm::vec3 forward() const;
    
    /**
     * @brief 获取右方向（本地 X 轴）
     */
    glm::vec3 right() const;
    
    /**
     * @brief 获取上方向（本地 Y 轴）
     */
    glm::vec3 up() const;

    // ===== 变换操作 =====
    
    /**
     * @brief 平移（本地坐标）
     * @param translation 平移向量
     */
    void translate(const glm::vec3& translation);
    
    /**
     * @brief 旋转（本地坐标）
     * @param eulerAngles 旋转欧拉角（度）
     */
    void rotate(const glm::vec3& eulerAngles);
    
    /**
     * @brief 围绕轴旋转（本地坐标）
     * @param axis 旋转轴
     * @param angle 旋转角度（度）
     */
    void rotateAround(const glm::vec3& axis, float angle);
    
    /**
     * @brief 看向目标点
     * @param target 目标世界坐标
     * @param up 上方向
     */
    void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0, 1, 0));

    // ===== 坐标转换 =====
    
    /**
     * @brief 将点从本地坐标转换到世界坐标
     */
    glm::vec3 transformPoint(const glm::vec3& point) const;
    
    /**
     * @brief 将方向从本地坐标转换到世界坐标
     */
    glm::vec3 transformDirection(const glm::vec3& direction) const;
    
    /**
     * @brief 将点从世界坐标转换到本地坐标
     */
    glm::vec3 inverseTransformPoint(const glm::vec3& point) const;
    
    /**
     * @brief 将方向从世界坐标转换到本地坐标
     */
    glm::vec3 inverseTransformDirection(const glm::vec3& direction) const;

    // Component 接口
    const char* getTypeName() const override { return "Transform"; }
};

#endif // TRANSFORM_H
