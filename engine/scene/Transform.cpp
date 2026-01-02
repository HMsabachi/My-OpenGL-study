// Transform.cpp - 变换组件实现
#include "Transform.h"
#include "GameObject.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

Transform::Transform(GameObject* owner)
    : Component(owner, "Transform"),
      m_localPosition(0.0f),
      m_localRotation(1.0f, 0.0f, 0.0f, 0.0f),  // 单位四元数
      m_localScale(1.0f),
      m_localToWorldMatrix(1.0f),
      m_worldToLocalMatrix(1.0f),
      m_matrixDirty(true) {
}

void Transform::markDirty() {
    m_matrixDirty = true;
    
    // 递归标记所有子对象的变换为脏
    if (m_owner) {
        for (size_t i = 0; i < m_owner->getChildCount(); ++i) {
            if (GameObject* child = m_owner->getChild(i)) {
                if (Transform* childTransform = child->getTransform()) {
                    childTransform->markDirty();
                }
            }
        }
    }
}

void Transform::updateMatrices() const {
    if (!m_matrixDirty) return;
    
    // 计算本地变换矩阵
    glm::mat4 localMatrix = getLocalMatrix();
    
    // 如果有父对象，组合父对象的变换
    if (m_owner && m_owner->getParent()) {
        Transform* parentTransform = m_owner->getParent()->getTransform();
        if (parentTransform) {
            m_localToWorldMatrix = parentTransform->getLocalToWorldMatrix() * localMatrix;
        } else {
            m_localToWorldMatrix = localMatrix;
        }
    } else {
        m_localToWorldMatrix = localMatrix;
    }
    
    // 计算逆矩阵
    m_worldToLocalMatrix = glm::inverse(m_localToWorldMatrix);
    
    m_matrixDirty = false;
}

// ===== 本地变换 =====

void Transform::setLocalPosition(const glm::vec3& position) {
    m_localPosition = position;
    markDirty();
}

void Transform::setLocalRotation(const glm::quat& rotation) {
    m_localRotation = rotation;
    markDirty();
}

void Transform::setLocalRotationEuler(const glm::vec3& eulerAngles) {
    m_localRotation = glm::quat(glm::radians(eulerAngles));
    markDirty();
}

glm::vec3 Transform::getLocalRotationEuler() const {
    return glm::degrees(glm::eulerAngles(m_localRotation));
}

void Transform::setLocalScale(const glm::vec3& scale) {
    m_localScale = scale;
    markDirty();
}

// ===== 世界变换 =====

glm::vec3 Transform::getWorldPosition() const {
    updateMatrices();
    return glm::vec3(m_localToWorldMatrix[3]);
}

void Transform::setWorldPosition(const glm::vec3& position) {
    if (m_owner && m_owner->getParent()) {
        Transform* parentTransform = m_owner->getParent()->getTransform();
        if (parentTransform) {
            m_localPosition = parentTransform->inverseTransformPoint(position);
        } else {
            m_localPosition = position;
        }
    } else {
        m_localPosition = position;
    }
    markDirty();
}

glm::quat Transform::getWorldRotation() const {
    if (m_owner && m_owner->getParent()) {
        Transform* parentTransform = m_owner->getParent()->getTransform();
        if (parentTransform) {
            return parentTransform->getWorldRotation() * m_localRotation;
        }
    }
    return m_localRotation;
}

void Transform::setWorldRotation(const glm::quat& rotation) {
    if (m_owner && m_owner->getParent()) {
        Transform* parentTransform = m_owner->getParent()->getTransform();
        if (parentTransform) {
            m_localRotation = glm::inverse(parentTransform->getWorldRotation()) * rotation;
        } else {
            m_localRotation = rotation;
        }
    } else {
        m_localRotation = rotation;
    }
    markDirty();
}

glm::vec3 Transform::getWorldScale() const {
    glm::vec3 worldScale = m_localScale;
    
    if (m_owner && m_owner->getParent()) {
        Transform* parentTransform = m_owner->getParent()->getTransform();
        if (parentTransform) {
            worldScale *= parentTransform->getWorldScale();
        }
    }
    
    return worldScale;
}

// ===== 变换矩阵 =====

const glm::mat4& Transform::getLocalToWorldMatrix() const {
    updateMatrices();
    return m_localToWorldMatrix;
}

const glm::mat4& Transform::getWorldToLocalMatrix() const {
    updateMatrices();
    return m_worldToLocalMatrix;
}

glm::mat4 Transform::getLocalMatrix() const {
    glm::mat4 matrix(1.0f);
    
    // T * R * S
    matrix = glm::translate(matrix, m_localPosition);
    matrix *= glm::toMat4(m_localRotation);
    matrix = glm::scale(matrix, m_localScale);
    
    return matrix;
}

// ===== 方向向量 =====

glm::vec3 Transform::forward() const {
    return glm::normalize(getWorldRotation() * glm::vec3(0, 0, -1));
}

glm::vec3 Transform::right() const {
    return glm::normalize(getWorldRotation() * glm::vec3(1, 0, 0));
}

glm::vec3 Transform::up() const {
    return glm::normalize(getWorldRotation() * glm::vec3(0, 1, 0));
}

// ===== 变换操作 =====

void Transform::translate(const glm::vec3& translation) {
    m_localPosition += translation;
    markDirty();
}

void Transform::rotate(const glm::vec3& eulerAngles) {
    glm::quat rotation = glm::quat(glm::radians(eulerAngles));
    m_localRotation = m_localRotation * rotation;
    markDirty();
}

void Transform::rotateAround(const glm::vec3& axis, float angle) {
    glm::quat rotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
    m_localRotation = m_localRotation * rotation;
    markDirty();
}

void Transform::lookAt(const glm::vec3& target, const glm::vec3& up) {
    glm::vec3 worldPos = getWorldPosition();
    glm::vec3 direction = glm::normalize(target - worldPos);
    
    if (glm::length(direction) < 0.0001f) {
        return;  // 目标点与当前位置重合
    }
    
    glm::mat4 lookAtMatrix = glm::lookAt(worldPos, target, up);
    glm::quat rotation = glm::quat_cast(glm::inverse(lookAtMatrix));
    
    setWorldRotation(rotation);
}

// ===== 坐标转换 =====

glm::vec3 Transform::transformPoint(const glm::vec3& point) const {
    updateMatrices();
    glm::vec4 result = m_localToWorldMatrix * glm::vec4(point, 1.0f);
    return glm::vec3(result);
}

glm::vec3 Transform::transformDirection(const glm::vec3& direction) const {
    updateMatrices();
    glm::vec4 result = m_localToWorldMatrix * glm::vec4(direction, 0.0f);
    return glm::vec3(result);
}

glm::vec3 Transform::inverseTransformPoint(const glm::vec3& point) const {
    updateMatrices();
    glm::vec4 result = m_worldToLocalMatrix * glm::vec4(point, 1.0f);
    return glm::vec3(result);
}

glm::vec3 Transform::inverseTransformDirection(const glm::vec3& direction) const {
    updateMatrices();
    glm::vec4 result = m_worldToLocalMatrix * glm::vec4(direction, 0.0f);
    return glm::vec3(result);
}
