// Object.cpp
#include "Object.h"
#include "../engine.h"
#include <glm/gtc/type_ptr.hpp>

/**
 * 构造函数实现。
 */
Object::Object(Engine* engine, const glm::vec3& position, const glm::vec3& velocity)
    : m_engine(engine), 
      m_position(position), 
      m_rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),  // 单位四元数
      m_scale(glm::vec3(1.0f)), 
      m_velocity(velocity), 
      m_isActive(true),
      m_physicsType(PhysicsType::NONE),
      m_collisionShape(CollisionShape::NONE),
      m_rigidBody(nullptr),
      m_collider(nullptr),
      m_collisionShapeObj(nullptr) {
}

/**
 * 析构函数实现。
 */
Object::~Object() {
    // 清理物理引擎资源
    if (m_rigidBody && m_engine && m_engine->pWorld) {
        // 先移除碰撞体
        if (m_collider) {
            m_rigidBody->removeCollider(m_collider);
            m_collider = nullptr;
        }
        // 销毁刚体
        m_engine->pWorld->destroyRigidBody(m_rigidBody);
        m_rigidBody = nullptr;
    }
    
    // 清理碰撞形状 - 使用 physicsCommon 的销毁方法
    if (m_collisionShapeObj && m_engine) {
        // 根据类型销毁碰撞形状
        switch (m_collisionShape) {
            case CollisionShape::BOX:
                m_engine->physicsCommon.destroyBoxShape(static_cast<rp3d::BoxShape*>(m_collisionShapeObj));
                break;
            case CollisionShape::SPHERE:
                m_engine->physicsCommon.destroySphereShape(static_cast<rp3d::SphereShape*>(m_collisionShapeObj));
                break;
            case CollisionShape::PLANE:
                // Plane 使用 BoxShape 实现
                m_engine->physicsCommon.destroyBoxShape(static_cast<rp3d::BoxShape*>(m_collisionShapeObj));
                break;
            case CollisionShape::CAPSULE:
                m_engine->physicsCommon.destroyCapsuleShape(static_cast<rp3d::CapsuleShape*>(m_collisionShapeObj));
                break;
            default:
                break;
        }
        m_collisionShapeObj = nullptr;
    }
}

/**
 * 更新实现。
 */
void Object::update(float deltaTime) {
    if (m_isActive) {
        // 如果有物理体，从物理引擎同步
        if (m_rigidBody && m_physicsType == PhysicsType::DYNAMIC) {
            syncFromPhysics();
        } else {
            // 否则使用简单的速度更新
            m_position += m_velocity * deltaTime;
        }
    }
}

/**
 * 销毁实现。
 */
void Object::destroy() {
    m_isActive = false;
}

// ===== 变换相关实现 =====

void Object::setPosition(const glm::vec3& position) {
    m_position = position;
    // 如果有物理体，同步到物理引擎
    if (m_rigidBody && m_physicsType != PhysicsType::DYNAMIC) {
        syncToPhysics();
    }
}

void Object::setRotation(const glm::quat& rotation) {
    m_rotation = rotation;
    // 如果有物理体，同步到物理引擎
    if (m_rigidBody && m_physicsType != PhysicsType::DYNAMIC) {
        syncToPhysics();
    }
}

void Object::setRotationEuler(const glm::vec3& eulerAngles) {
    m_rotation = glm::quat(glm::radians(eulerAngles));
    // 如果有物理体，同步到物理引擎
    if (m_rigidBody && m_physicsType != PhysicsType::DYNAMIC) {
        syncToPhysics();
    }
}

void Object::setScale(const glm::vec3& scale) {
    m_scale = scale;
}

void Object::setVelocity(const glm::vec3& velocity) {
    m_velocity = velocity;
}

// ===== 物理引擎相关实现 =====

void Object::initPhysics(PhysicsType type, CollisionShape shape, const glm::vec3& shapeSize, float mass) {
    if (!m_engine || !m_engine->pWorld) {
        return;
    }

    m_physicsType = type;
    m_collisionShape = shape;

    // 创建变换
    rp3d::Vector3 rp3dPosition(m_position.x, m_position.y, m_position.z);
    rp3d::Quaternion rp3dRotation(m_rotation.x, m_rotation.y, m_rotation.z, m_rotation.w);
    rp3d::Transform transform(rp3dPosition, rp3dRotation);

    // 创建刚体
    m_rigidBody = m_engine->pWorld->createRigidBody(transform);

    // 设置刚体类型
    switch (type) {
        case PhysicsType::STATIC:
            m_rigidBody->setType(rp3d::BodyType::STATIC);
            break;
        case PhysicsType::KINEMATIC:
            m_rigidBody->setType(rp3d::BodyType::KINEMATIC);
            break;
        case PhysicsType::DYNAMIC:
            m_rigidBody->setType(rp3d::BodyType::DYNAMIC);
            break;
        default:
            break;
    }

    // 创建碰撞形状
    switch (shape) {
        case CollisionShape::BOX: {
            rp3d::Vector3 halfExtents(shapeSize.x * 0.5f, shapeSize.y * 0.5f, shapeSize.z * 0.5f);
            m_collisionShapeObj = m_engine->physicsCommon.createBoxShape(halfExtents);
            break;
        }
        case CollisionShape::SPHERE: {
            m_collisionShapeObj = m_engine->physicsCommon.createSphereShape(shapeSize.x);
            break;
        }
        case CollisionShape::PLANE: {
            // 平面使用 BoxShape 模拟（薄的盒子）
            rp3d::Vector3 halfExtents(shapeSize.x * 0.5f, 0.1f, shapeSize.z * 0.5f);
            m_collisionShapeObj = m_engine->physicsCommon.createBoxShape(halfExtents);
            break;
        }
        case CollisionShape::CAPSULE: {
            m_collisionShapeObj = m_engine->physicsCommon.createCapsuleShape(shapeSize.x, shapeSize.y);
            break;
        }
        default:
            break;
    }

    // 添加碰撞体到刚体
    if (m_collisionShapeObj) {
        rp3d::Transform colliderTransform = rp3d::Transform::identity();
        m_collider = m_rigidBody->addCollider(m_collisionShapeObj, colliderTransform);
        
        // 设置质量（仅对动态物体有效）
        if (type == PhysicsType::DYNAMIC) {
            m_collider->getMaterial().setMassDensity(mass);
            m_rigidBody->updateMassPropertiesFromColliders();
        }
    }
}

void Object::syncFromPhysics() {
    if (!m_rigidBody) return;

    const rp3d::Transform& transform = m_rigidBody->getTransform();
    const rp3d::Vector3& rp3dPos = transform.getPosition();
    const rp3d::Quaternion& rp3dRot = transform.getOrientation();

    m_position = glm::vec3(rp3dPos.x, rp3dPos.y, rp3dPos.z);
    m_rotation = glm::quat(rp3dRot.w, rp3dRot.x, rp3dRot.y, rp3dRot.z);
}

void Object::syncToPhysics() {
    if (!m_rigidBody) return;

    rp3d::Vector3 rp3dPosition(m_position.x, m_position.y, m_position.z);
    rp3d::Quaternion rp3dRotation(m_rotation.x, m_rotation.y, m_rotation.z, m_rotation.w);
    rp3d::Transform transform(rp3dPosition, rp3dRotation);

    m_rigidBody->setTransform(transform);
}