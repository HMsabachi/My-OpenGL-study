#include "camera.h"
#include "object/object.h"  // 引入 Object 类定义
#include "../application/application.h"
#include <algorithm>

Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up,
    float fov, float aspect, float near, float far)
    : m_position(position),
    m_target(target),
    m_up(up),
    m_fov(fov),
    m_aspect(aspect),
    m_near(near),
    m_far(far),
    m_isPerspective(true) {
    updateProjection();
}

glm::mat4 Camera::getViewMatrix() const {
    if (m_useFPS) {
        return glm::lookAt(m_position, m_position + m_front, m_up);
    }
    return glm::lookAt(m_position, m_target, m_up);
}

glm::mat4 Camera::getProjectionMatrix() const {
    return m_projection;
}

void Camera::setPosition(const glm::vec3& position) {
    m_position = position;
}

glm::vec3 Camera::getPosition() const {
    return m_position;
}

void Camera::setTarget(const glm::vec3& target) {
    m_target = target;
}

glm::vec3 Camera::getTarget() const {
    return m_target;
}

void Camera::setUp(const glm::vec3& up) {
    m_up = up;
}

glm::vec3 Camera::getUp() const {
    return m_up;
}

void Camera::translate(const glm::vec3& offset) {
    m_position += offset;
    m_target += offset;
}

void Camera::rotateAroundTarget(float angle, const glm::vec3& axis) {
    glm::vec3 direction = m_position - m_target;
    glm::mat4 rotation =
        glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis);

    direction = glm::vec3(rotation * glm::vec4(direction, 1.0f));
    m_position = m_target + direction;
    m_up = glm::vec3(rotation * glm::vec4(m_up, 0.0f));
}

void Camera::setFOV(float fov) {
    m_fov = fov;
    if (m_isPerspective) updateProjection();
}

void Camera::setAspect(float aspect) {
    m_aspect = aspect;
    updateProjection();
}

void Camera::setNearFar(float near, float far) {
    m_near = near;
    m_far = far;
    updateProjection();
}

void Camera::setOrthographic(float left, float right,
    float bottom, float top,
    float near, float far) {
    m_isPerspective = false;
    m_projection = glm::ortho(left, right, bottom, top, near, far);
}

void Camera::updateProjection() {
    if (m_isPerspective) {
        m_projection = glm::perspective(
            glm::radians(m_fov), m_aspect, m_near, m_far);
    }
}

// ================= FPS Camera 实现 =================

void Camera::enableFPS(bool enable) {
    m_useFPS = enable;

    if (enable) {
        glm::vec3 dir = glm::normalize(m_target - m_position);
        m_pitch = glm::degrees(asin(dir.y));
        m_yaw = glm::degrees(atan2(dir.z, dir.x));
        updateFPSVectors();
    }
}

void Camera::setYaw(float yaw) {
    m_yaw = yaw;
    if (m_useFPS) updateFPSVectors();
}

void Camera::setPitch(float pitch) {
    m_pitch = pitch;
    if (m_useFPS) updateFPSVectors();
}

void Camera::setRoll(float roll) {
    m_roll = roll;
    if (m_useFPS) updateFPSVectors();
}

float Camera::getYaw() const { return m_yaw; }
float Camera::getPitch() const { return m_pitch; }
float Camera::getRoll() const { return m_roll; }

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    if (!m_useFPS) return;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    if (constrainPitch) {
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }

    updateFPSVectors();
}

void Camera::moveForward(float delta) {
    if (m_useFPS)
        m_position += glm::normalize(glm::vec3(m_front.x, 0.0f, m_front.z)) * delta;
}

void Camera::moveRight(float delta) {
    if (m_useFPS)
        m_position += glm::normalize(glm::vec3(m_right.x, 0.0f, m_right.z)) * delta;
}

void Camera::moveUpFPS(float delta) {
    if (m_useFPS)
        m_position += m_worldUp * delta;
}

void Camera::updateFPSVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);

    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));

    // Roll（可选）
    if (m_roll != 0.0f) {
        glm::mat4 rollMat =
            glm::rotate(glm::mat4(1.0f),
                glm::radians(m_roll),
                m_front);
        m_up = glm::vec3(rollMat * glm::vec4(m_up, 0.0f));
        m_right = glm::normalize(glm::cross(m_front, m_up));
    }

    // 同步 target，保证旧接口逻辑正确
    m_target = m_position + m_front;
}

// ================= LookAt 方法实现 =================

void Camera::lookAt(const glm::vec3& target, bool smooth) {
    if (m_useFPS) {
        // FPS 模式：计算朝向目标的 Yaw 和 Pitch
        glm::vec3 direction = glm::normalize(target - m_position);
        
        if (smooth) {
            // 平滑过渡：插值到目标角度
            float targetYaw = glm::degrees(atan2(direction.z, direction.x));
            float targetPitch = glm::degrees(asin(direction.y));
            
            // 简单线性插值，可以调整插值速度
            float lerpFactor = 0.05f;
            m_yaw = glm::mix(m_yaw, targetYaw, lerpFactor);
            m_pitch = glm::mix(m_pitch, targetPitch, lerpFactor);
        } else {
            // 立即转向
            m_yaw = glm::degrees(atan2(direction.z, direction.x));
            m_pitch = glm::degrees(asin(direction.y));
            
            // 限制 Pitch 范围
            m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
        }
        
        updateFPSVectors();
    } else {
        // LookAt 模式：直接设置目标点
        m_target = target;
    }
}

void Camera::lookAt(const Object* object, bool smooth) {
    if (object != nullptr) {
        lookAt(object->getPosition(), smooth);
    }
}
