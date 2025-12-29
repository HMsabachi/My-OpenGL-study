// Object.cpp
#include "Object.h"

/**
 * 构造函数实现。
 */
Object::Object(const glm::vec3& position, const glm::vec3& velocity)
    : m_position(position), m_velocity(velocity), m_isActive(true), m_rigidBody(nullptr) {
    // 这里可以初始化ReactPhysics3D的刚体，如果需要
    // 例如：m_rigidBody = physicsWorld->createRigidBody(transform);
}

/**
 * 析构函数实现。
 */
Object::~Object() {
    // 清理物理引擎资源，如果适用
    // 例如：if (m_rigidBody) physicsWorld->destroyRigidBody(m_rigidBody);
}

/**
 * 更新实现。
 */
void Object::update(float deltaTime) {
    if (m_isActive) {
        m_position += m_velocity * deltaTime;
        // 如果使用ReactPhysics3D，可以在这里同步物理模拟
        // 例如：m_position = m_rigidBody->getTransform().getPosition();
    }
}

/**
 * 销毁实现。
 */
void Object::destroy() {
    m_isActive = false;
}