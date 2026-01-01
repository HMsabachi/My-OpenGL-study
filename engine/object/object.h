// Object.h
#ifndef OBJECT_H
#define OBJECT_H

#include <glm/glm.hpp>  // 用于向量和矩阵操作，假设使用GLM库
#include <glm/gtc/quaternion.hpp>  // 四元数用于旋转
#include <reactphysics3d/reactphysics3d.h>  // ReactPhysics3D物理引擎
#include <string>  // 添加string头文件

/**
 * 游戏物体基类，提供基本的位置、速度、更新和渲染功能。
 * 子类可以扩展以实现特定行为，如玩家、敌人等。
 */

class Engine;

class Object {
public:
    /**
     * 物理体类型枚举
     */
    enum class PhysicsType {
        NONE,        // 无物理模拟
        STATIC,      // 静态物体（不移动，如地板、墙壁）
        KINEMATIC,   // 运动学物体（程序控制移动，不受物理影响）
        DYNAMIC      // 动态物体（受物理引擎完全控制）
    };

    /**
     * 碰撞形状类型枚举
     */
    enum class CollisionShape {
        NONE,
        BOX,
        SPHERE,
        PLANE,
        CAPSULE
    };

    /**
     * 构造函数，初始化位置和速度。
     * @param engine 游戏引擎实例指针
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

    // ===== 变换相关 =====
    
    // 获取位置
    const glm::vec3& getPosition() const { return m_position; }
    
    // 设置位置
    void setPosition(const glm::vec3& position);

    // 获取旋转（四元数）
    const glm::quat& getRotation() const { return m_rotation; }
    
    // 设置旋转（四元数）
    void setRotation(const glm::quat& rotation);
    
    // 设置旋转（欧拉角）
    void setRotationEuler(const glm::vec3& eulerAngles);
    
    // 获取缩放
    const glm::vec3& getScale() const { return m_scale; }
    
    // 设置缩放
    void setScale(const glm::vec3& scale);

    // 获取速度
    const glm::vec3& getVelocity() const { return m_velocity; }

    // 设置速度
    void setVelocity(const glm::vec3& velocity);

    // 检查是否活跃
    bool isActive() const { return m_isActive; }

    // ===== 名称相关 =====
    
    /**
     * 获取对象名称
     * @return 对象名称
     */
    const std::string& getName() const { return m_name; }
    
    /**
     * 设置对象名称
     * @param name 新的名称
     */
    void setName(const std::string& name) { m_name = name; }

    // ===== 物理引擎相关 =====
    
    /**
     * 初始化物理体
     * @param type 物理体类型
     * @param shape 碰撞形状类型
     * @param shapeSize 形状大小参数（对于Box是半尺寸，对于Sphere是半径等）
     * @param mass 质量（仅对DYNAMIC类型有效）
     */
    virtual void initPhysics(PhysicsType type, CollisionShape shape, const glm::vec3& shapeSize, float mass = 1.0f);
    
    /**
     * 从物理引擎同步变換信息到渲染
     */
    void syncFromPhysics();
    
    /**
     * 将变换信息同步到物理引擎
     */
    void syncToPhysics();
    
    /**
     * 获取物理刚体指针
     */
    rp3d::RigidBody* getRigidBody() const { return m_rigidBody; }
    
    /**
     * 获取碰撞体指针
     */
    rp3d::Collider* getCollider() const { return m_collider; }
    
    /**
     * 施加力到物体（纯虚函数，子类必须实现）
     * @param force 施加的力向量
     */
    virtual void applyForce(const glm::vec3& force) = 0;

protected:
    Engine* m_engine;          // 游戏引擎实例
    glm::vec3 m_position;      // 位置
    glm::quat m_rotation;      // 旋转（四元数）
    glm::vec3 m_scale;         // 缩放
    glm::vec3 m_velocity;      // 速度
    bool m_isActive;           // 活跃标志
    std::string m_name;        // 对象名称

    // ReactPhysics3D相关
    PhysicsType m_physicsType;              // 物理体类型
    CollisionShape m_collisionShape;        // 碰撞形状类型
    rp3d::RigidBody* m_rigidBody;          // 物理引擎刚体
    rp3d::Collider* m_collider;            // 碰撞体
    rp3d::CollisionShape* m_collisionShapeObj;  // 碰撞形状对象

private:
    static int s_objectCounter;  // 对象计数器，用于生成唯一名称
};

#endif // OBJECT_H