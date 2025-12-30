#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H

#include <glm/glm.hpp>
#include "object/object.h"

class Camera;
class Engine;

/**
 * @brief 玩家控制器 - 支持切换控制摄像机或物体
 */
class PlayerController {
public:
    enum class ControlMode {
        CAMERA,    // 控制摄像机
        OBJECT     // 控制物体
    };

    PlayerController(Engine* engine, Camera* camera);
    ~PlayerController() = default;

    // 更新
    void update(float deltaTime);

    // 控制模式
    void toggleControlMode();  // 切换控制模式
    void setControlMode(ControlMode mode) { m_controlMode = mode; }
    ControlMode getControlMode() const { return m_controlMode; }

    // 控制对象
    void setControlledObject(Object* object) { m_controlledObject = object; }
    Object* getControlledObject() const { return m_controlledObject; }

    // 移动参数
    void setMoveSpeed(float speed) { m_moveSpeed = speed; }
	void setMoveForce(float force) { m_moveForce = force; }
    float getMoveSpeed() const { return m_moveSpeed; }

private:
    Engine* m_engine;
    Camera* m_camera;
    Object* m_controlledObject;  // 当前控制的物体
    ControlMode m_controlMode;    // 当前控制模式

    // 移动参数
    float m_moveSpeed;            // 移动速度
    float m_moveForce;            // 施加的力（用于物理物体）

    // 内部更新方法
    void updateCameraControl(float deltaTime);
    void updateObjectControl(float deltaTime);
};

#endif // PLAYER_CONTROLLER_H
