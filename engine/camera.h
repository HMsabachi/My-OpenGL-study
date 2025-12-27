#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * @class Camera
 * @brief 相机类，用于OpenGL渲染引擎中管理视图和投影矩阵。
 *
 * 支持传统 LookAt 相机 + 可选 FPS 自由旋转相机（Yaw / Pitch / Roll）
 */
class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float fov = 45.0f,
        float aspect = 1.0f,
        float near = 0.1f,
        float far = 100.0f);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;

    void setPosition(const glm::vec3& position);
    glm::vec3 getPosition() const;

    void setTarget(const glm::vec3& target);
    glm::vec3 getTarget() const;

    void setUp(const glm::vec3& up);
    glm::vec3 getUp() const;

    void translate(const glm::vec3& offset);
    void rotateAroundTarget(float angle, const glm::vec3& axis);

    void setFOV(float fov);
    void setAspect(float aspect);
    void setNearFar(float near, float far);
    void setOrthographic(float left, float right,
        float bottom, float top,
        float near, float far);

    // ===== FPS Camera 扩展（新增，不影响旧接口） =====
    void enableFPS(bool enable);

    void setYaw(float yaw);
    void setPitch(float pitch);
    void setRoll(float roll);

    float getYaw() const;
    float getPitch() const;
    float getRoll() const;

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    void moveForward(float delta);
    void moveRight(float delta);
    void moveUpFPS(float delta);

private:
    // ===== 原有成员 =====
    glm::vec3 m_position;
    glm::vec3 m_target;
    glm::vec3 m_up;

    float m_fov;
    float m_aspect;
    float m_near;
    float m_far;

    bool m_isPerspective;
    glm::mat4 m_projection;

    void updateProjection();

    // ===== FPS Camera 成员（新增） =====
    bool m_useFPS = false;

    float m_yaw = -90.0f;
    float m_pitch = 0.0f;
    float m_roll = 0.0f;

    glm::vec3 m_front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 m_worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    void updateFPSVectors();
};

#endif // CAMERA_H
