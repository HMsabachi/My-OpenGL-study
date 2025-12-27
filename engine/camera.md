下面是一份**完整、工程级、可直接给项目成员看的 Camera 使用文档**，内容**严格对应你当前这版 Camera（LookAt + 可选 FPS 扩展）**，不假设读者了解实现细节。

---

# Camera 使用文档

**（LookAt 相机 + FPS 自由旋转扩展）**

---

## 1. 类概述

`Camera` 是一个用于 OpenGL 渲染的相机类，支持两种工作模式：

### ① LookAt 模式（默认）

* 通过 **位置（Position）+ 目标点（Target）+ 向上向量（Up）**
* 适合：

  * 场景浏览
  * 模型查看器
  * 围绕目标旋转

### ② FPS 模式（可选开启）

* 通过 **Yaw / Pitch / Roll 欧拉角**
* 相机朝向由自身角度决定
* 适合：

  * 第一人称相机
  * 自由飞行相机
  * 游戏视角

👉 **两种模式可随时切换，互不影响**

---

## 2. 构造函数

```cpp
Camera(
    glm::vec3 position = {0,0,3},
    glm::vec3 target   = {0,0,0},
    glm::vec3 up       = {0,1,0},
    float fov = 45.0f,
    float aspect = 1.0f,
    float near = 0.1f,
    float far = 100.0f
);
```

### 参数说明

| 参数         | 含义       |
| ---------- | -------- |
| position   | 相机世界坐标   |
| target     | 相机看向的目标点 |
| up         | 相机向上方向   |
| fov        | 透视视角（角度） |
| aspect     | 宽高比      |
| near / far | 裁剪平面     |

---

## 3. 矩阵获取（核心）

### 3.1 视图矩阵

```cpp
glm::mat4 view = camera.getViewMatrix();
```

* LookAt 模式：
  `lookAt(position, target, up)`
* FPS 模式：
  `lookAt(position, position + front, up)`

> 无需关心内部模式，接口统一

---

### 3.2 投影矩阵

```cpp
glm::mat4 proj = camera.getProjectionMatrix();
```

* 默认：透视投影
* 可切换为正交投影

---

## 4. LookAt 模式用法（原有功能）

### 4.1 设置位置 / 目标 / Up

```cpp
camera.setPosition({0, 3, 5});
camera.setTarget({0, 0, 0});
camera.setUp({0, 1, 0});
```

---

### 4.2 平移相机

```cpp
camera.translate({1.0f, 0.0f, 0.0f});
```

* 会同时平移 `position` 和 `target`
* 适合整体相机移动

---

### 4.3 围绕目标旋转

```cpp
camera.rotateAroundTarget(30.0f, {0, 1, 0});
```

* 绕 `target` 旋转
* 常用于模型查看器

---

## 5. FPS 模式（新增功能）

### 5.1 启用 / 关闭 FPS 模式

```cpp
camera.enableFPS(true);   // 开启 FPS
camera.enableFPS(false);  // 回到 LookAt
```

* 开启时会自动根据当前 `position → target` 计算初始角度
* 旧代码仍然可用

---

### 5.2 欧拉角说明

| 角度    | 含义            |
| ----- | ------------- |
| Yaw   | 偏航（左右看，绕 Y 轴） |
| Pitch | 俯仰（上下看，绕 X 轴） |
| Roll  | 滚转（歪头，绕 Z 轴）  |

单位：**度**

---

### 5.3 设置 / 获取角度

```cpp
camera.setYaw(-90.0f);
camera.setPitch(10.0f);
camera.setRoll(0.0f);

float yaw   = camera.getYaw();
float pitch = camera.getPitch();
float roll  = camera.getRoll();
```

⚠ 建议 **Pitch ∈ [-89°, 89°]**，避免万向节问题

---

### 5.4 鼠标控制（最常用）

```cpp
camera.processMouseMovement(dx, dy);
```

* `dx`：鼠标 X 方向偏移
* `dy`：鼠标 Y 方向偏移
* 内部已处理灵敏度和 Pitch 限制

> **这是 FPS 旋转的核心接口**

---

### 5.5 FPS 移动（基于相机自身方向）

```cpp
camera.moveForward(speed); // 前 / 后
camera.moveRight(speed);   // 左 / 右
camera.moveUpFPS(speed);   // 上 / 下
```

| 函数          | 方向    |
| ----------- | ----- |
| moveForward | Front |
| moveRight   | Right |
| moveUpFPS   | 世界 Up |

---

## 6. 投影控制

### 6.1 设置透视参数

```cpp
camera.setFOV(60.0f);
camera.setAspect(width / height);
camera.setNearFar(0.1f, 500.0f);
```

---

### 6.2 切换正交投影

```cpp
camera.setOrthographic(
    -10, 10,
    -10, 10,
    0.1f, 100.0f
);
```

---

## 7. 推荐使用流程（FPS）

```cpp
// 初始化
Camera camera;
camera.enableFPS(true);

// 主循环
while (running) {
    camera.processMouseMovement(dx, dy);

    if (W) camera.moveForward(speed);
    if (S) camera.moveForward(-speed);
    if (A) camera.moveRight(-speed);
    if (D) camera.moveRight(speed);

    shader.setMat4("view", camera.getViewMatrix());
}
```

---

## 8. 模式对比总结

| 特性     | LookAt | FPS  |
| ------ | ------ | ---- |
| target | 必须     | 自动计算 |
| 欧拉角    | ❌      | ✅    |
| 鼠标控制   | ❌      | ✅    |
| 第一人称   | ❌      | ✅    |
| 兼容旧代码  | ✅      | ✅    |

---

## 9. 注意事项（必读）

1. **不要同时混用 LookAt 旋转 + FPS 移动**
2. FPS 模式下请使用：

   * `processMouseMovement`
   * `moveForward / moveRight`
3. Pitch 不建议超过 ±89°
4. Roll 默认关闭（0），开启后会有“歪头”效果

---

## 10. 后续可扩展方向

* 🔹 四元数相机（彻底消除万向节死锁）
* 🔹 地面行走（锁 Y）
* 🔹 相机平滑（阻尼 / 插值）
* 🔹 摄像机碰撞检测

---

## 11. 结语

> 这是一个**工程安全、零破坏、可渐进升级**的相机设计。
> 你可以把它当：
>
> * LookAt 相机 ✔
> * FPS 相机 ✔
> * 飞行相机 ✔

如果你需要，我可以下一步**直接按 GLFW / SDL 输入系统给你写完整示例代码**。
