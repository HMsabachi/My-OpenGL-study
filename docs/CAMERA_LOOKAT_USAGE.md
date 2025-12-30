# Camera LookAt 使用说明

## 功能概述

为 `Camera` 类添加了 `lookAt` 方法，使相机能够看向指定的坐标或游戏对象。

## 方法签名

```cpp
// 1. 看向指定坐标
void lookAt(const glm::vec3& target, bool smooth = false);

// 2. 看向游戏对象
void lookAt(const Object* object, bool smooth = false);
```

## 参数说明

- **target**: 目标坐标（世界空间）
- **object**: 目标对象指针
- **smooth**: 是否启用平滑过渡
  - `false`（默认）：立即转向目标
  - `true`：平滑插值到目标方向

## 使用示例

### 1. 基础用法 - 看向坐标

```cpp
// 立即看向原点
camera->lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

// 平滑看向某个位置
camera->lookAt(glm::vec3(10.0f, 5.0f, -3.0f), true);
```

### 2. 看向游戏对象

```cpp
// 假设有一个球体对象
Sphere* sphere = new Sphere(this, glm::vec3(5.0f, 0.0f, 0.0f), 1.0f, shader, texture);

// 让相机看向球体（立即）
camera->lookAt(sphere);

// 平滑跟踪球体
camera->lookAt(sphere, true);
```

### 3. 在 Engine::update() 中持续跟踪对象

```cpp
void Engine::update()
{
    float deltaTime = myApp->getDeltaTime();
    setAcceleration(this->cameraData.acceleration);
    this->updateCamera(deltaTime);
    
    // 示例：持续平滑跟踪某个对象
    if (targetObject != nullptr) {
        camera->lookAt(targetObject, true);
    }
}
```

### 4. 结合键盘控制切换跟踪目标

```cpp
void Engine::keyCallback(int key, int action, int mods)
{
    auto self = myApp->engine;
    
    if (action != GLFW_PRESS) return;
    switch (key)
    {
        case GLFW_KEY_1:
            // 看向第一个对象
            self->camera->lookAt(self->scene->getObject(0));
            break;
            
        case GLFW_KEY_2:
            // 看向第二个对象
            self->camera->lookAt(self->scene->getObject(1));
            break;
            
        case GLFW_KEY_0:
            // 看向原点
            self->camera->lookAt(glm::vec3(0.0f, 0.0f, 0.0f));
            break;
    }
}
```

## 工作模式说明

### LookAt 模式（enableFPS(false)）

- 直接设置 `m_target` 成员
- 适合固定视角观察
- 不受 Yaw/Pitch 影响

### FPS 模式（enableFPS(true)）

- 计算并更新 Yaw 和 Pitch 角度
- 朝向目标的方向
- Pitch 自动限制在 [-89°, 89°] 范围内
- 适合第一人称视角跟踪

## 平滑过渡原理

启用 `smooth = true` 时：

```cpp
float lerpFactor = 0.1f;  // 插值速度，越大转向越快
m_yaw = glm::mix(m_yaw, targetYaw, lerpFactor);
m_pitch = glm::mix(m_pitch, targetPitch, lerpFactor);
```

- 插值速度为 0.1，表示每次调用移动 10% 的差距
- 在主循环中持续调用可实现平滑跟踪效果
- 可以修改 `lerpFactor` 调整跟踪速度

## 注意事项

1. **空指针检查**：传入对象指针时会自动检查是否为 `nullptr`
2. **模式兼容**：同时支持 LookAt 和 FPS 两种相机模式
3. **Pitch 限制**：FPS 模式下自动限制俯仰角，避免万向节死锁
4. **性能考虑**：
   - 立即转向（smooth=false）：调用一次即可
   - 平滑跟踪（smooth=true）：需要在每帧调用

## 完整示例：制作追踪相机

```cpp
// 在 Engine 类中添加成员
class Engine {
    // ...
    Object* trackingTarget = nullptr;
    bool isTracking = false;
};

// 在 update 中持续跟踪
void Engine::update()
{
    float deltaTime = myApp->getDeltaTime();
    
    if (isTracking && trackingTarget != nullptr) {
        camera->lookAt(trackingTarget, true);  // 平滑跟踪
    }
    
    setAcceleration(this->cameraData.acceleration);
    this->updateCamera(deltaTime);
}

// 在键盘回调中切换跟踪
void Engine::keyCallback(int key, int action, int mods)
{
    auto self = myApp->engine;
    
    if (action != GLFW_PRESS) return;
    switch (key)
    {
        case GLFW_KEY_T:
            // 切换跟踪模式
            self->isTracking = !self->isTracking;
            break;
            
        case GLFW_KEY_1:
        case GLFW_KEY_2:
        case GLFW_KEY_3:
            // 设置跟踪目标
            int index = key - GLFW_KEY_1;
            self->trackingTarget = self->scene->getObject(index);
            break;
    }
}
```

## 扩展建议

如果需要更高级的功能，可以考虑：

1. **环绕视角**：在 lookAt 基础上添加距离和角度偏移
2. **跟踪延迟**：添加时间延迟实现摄像机慢半拍的效果
3. **预测位置**：根据对象速度预测未来位置
4. **多目标插值**：在多个对象之间平滑切换

---

**添加时间**: 2024
**版本**: 1.0
**兼容性**: 支持 LookAt 和 FPS 两种相机模式
