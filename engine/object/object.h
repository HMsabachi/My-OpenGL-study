// Object.h
#ifndef OBJECT_H
#define OBJECT_H

#include <glm/glm.hpp>  // 用于向量和矩阵操作，假设使用GLM库
#include <reactphysics3d/reactphysics3d.h>  // ReactPhysics3D物理引擎

/**
 * 游戏物体基类，提供基本的位置、速度、更新和渲染功能。
 * 子类可以扩展以实现特定行为，如玩家、敌人等。
 */

class Engine;

class Object {
public:
    /**
     * 构造函数，初始化位置和速度。
     * @param position 初始位置 (glm::vec3)
     * @param velocity 初始速度 (glm::vec3)
     */
    Object(Engine* engine, const glm::vec3& position = glm::vec3(0.0f), const glm::vec3& velocity = glm::vec3(0.0f));

    /**
     * 析构函数。
     */
    virtual ~Object();

    /**
     * 更新物体状态，默认基于速度移动。
     * @param deltaTime 自上次更新以来经过的时间 (float)
     */
    virtual void update(float deltaTime);

    /**
     * 在屏幕上渲染物体。此方法为占位符，子类需重写以绘制实际模型或精灵。
     */
    virtual void render() const = 0;

    /**
     * 检查与另一个物体的碰撞。此方法为占位符，子类需根据形状实现（如AABB、球体）。
     * @param other 另一个Object实例
     * @return 如果碰撞则返回true，否则false
     */
    virtual bool collideWith(const Object& other) const = 0;

    /**
     * 标记物体为非活跃状态，以便从游戏循环中移除。
     */
    void destroy();

    // 获取位置
    const glm::vec3& getPosition() const { return m_position; }

    // 设置位置
    void setPosition(const glm::vec3& position) { m_position = position; }

    // 获取速度
    const glm::vec3& getVelocity() const { return m_velocity; }

    // 设置速度
    void setVelocity(const glm::vec3& velocity) { m_velocity = velocity; }

    // 检查是否活跃
    bool isActive() const { return m_isActive; }

protected:
    Engine* m_engine;     // 游戏引擎实例
    glm::vec3 m_position;  // 位置
    glm::vec3 m_velocity;  // 速度
    bool m_isActive;       // 活跃标志

    // ReactPhysics3D相关：可选的刚体指针，如果需要物理模拟
    rp3d::RigidBody* m_rigidBody;  // 物理引擎刚体（如果使用）
};

#endif // OBJECT_H