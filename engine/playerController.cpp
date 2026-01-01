#include "playerController.h"
#include "engine.h"
#include "camera.h"
#include "../application/application.h"
#include <iostream>

PlayerController::PlayerController(Engine* engine, Camera* camera)
    : m_engine(engine),
      m_camera(camera),
      m_controlledObject(nullptr),
      m_controlMode(ControlMode::CAMERA),
      m_moveSpeed(5.0f),
      m_moveForce(5.0f)  // 使用 m_moveSpeed 初始化
{
}

void PlayerController::update(float deltaTime) {
    if (m_controlMode == ControlMode::CAMERA) {
        updateCameraControl(deltaTime);
    } else if (m_controlMode == ControlMode::OBJECT) {
        updateObjectControl(deltaTime);
    }
}

void PlayerController::toggleControlMode() {
    if (m_controlMode == ControlMode::CAMERA) {
        if (m_controlledObject) {
            m_controlMode = ControlMode::OBJECT;
            std::cout << "切换到物体控制模式" << std::endl;
        } else {
            std::cout << "没有绑定控制对象！" << std::endl;
        }
    } else {
        m_controlMode = ControlMode::CAMERA;
        std::cout << "切换到摄像机控制模式" << std::endl;
    }
}

void PlayerController::updateCameraControl(float deltaTime) {
    // 摄像机控制由 Engine 的 updateCamera 处理
    // 这里不需要额外操作
}

void PlayerController::updateObjectControl(float deltaTime) {
    if (!m_controlledObject) return;

    auto window = myApp->getWindow();
    glm::vec3 moveDirection(0.0f);

    // 获取摄像机的前方和右方向（用于相对移动）
    glm::vec3 cameraForward = m_camera->getFront();
    glm::vec3 cameraRight = m_camera->getRight();
    
    // 投影到水平面（去掉Y分量）
    cameraForward.y = 0.0f;
    cameraRight.y = 0.0f;
    if (glm::length(cameraForward) > 0.001f) {
        cameraForward = glm::normalize(cameraForward);
    }
    if (glm::length(cameraRight) > 0.001f) {
        cameraRight = glm::normalize(cameraRight);
    }

    // WASD 控制
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        moveDirection += cameraForward;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        moveDirection -= cameraForward;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        moveDirection -= cameraRight;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        moveDirection += cameraRight;
    }

    // 垂直控制
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        moveDirection.y += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        moveDirection.y -= 1.0f;
    }

    // 归一化移动方向
    if (glm::length(moveDirection) > 0.001f) {
        moveDirection = glm::normalize(moveDirection);

        // 修复：同时使用速度和力，确保有响应
        // 对于史莱姆这种粒子系统，需要施加持续的力
        glm::vec3 force = moveDirection * m_moveForce;
        m_controlledObject->applyForce(force);
        
        // 可选：直接修改速度，让控制更响应
        // 对于传统刚体，这样做会绕过物理模拟，但对粒子系统很有效
        // m_controlledObject->setVelocity(moveDirection * m_moveSpeed);
    }
}
