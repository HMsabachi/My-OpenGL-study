# 🟢 史莱姆系统使用指南

## 📋 快速开始

史莱姆类已经创建完成并集成到项目中，使用基于物理的粒子系统模拟液态效果。

---

## 🎮 玩家控制器 **NEW!**

### 控制模式

按 **`C`** 键可以切换两种控制模式：

#### 1. 摄像机模式（默认）
- **WASD** - 移动摄像机
- **空格/Shift** - 上升/下降
- **鼠标** - 旋转视角
- **Alt** - 切换鼠标捕获

#### 2. 物体控制模式
- **WASD** - 施加力移动史莱姆（相对摄像机方向）
- **空格/Shift** - 向上/向下施加力
- **鼠标** - 仍可旋转视角
- **Alt** - 切换鼠标捕获

### 绑定控制对象

```cpp
// 在 setupDemoData() 中
Slime* slime = new Slime(...);
scene->addObject(slime);

// 绑定到玩家控制器
playerController->setControlledObject(slime);
playerController->setMoveSpeed(500.0f);  // 史莱姆需要更大的力度（推荐 300-800）
```

### 控制参数调整

```cpp
// 移动力度（施加在物体上的力）
playerController->setMoveSpeed(500.0f);  // 默认 500.0

// 不同物体推荐值：
// 单个物理对象（Cube, Sphere）: 50-150
// 史莱姆（280个粒子）: 300-800
// 值越大，移动越快
```

---

## 🎯 核心特性

✅ **物理模拟** - 每个粒子是独立的物理刚体
✅ **碰撞检测** - 粒子之间、粒子与场景物体自动碰撞
✅ **向心力系统** - 保持史莱姆形状的聚合力
✅ **流动性** - 低摩擦力实现液态效果
✅ **可调参数** - 灵活配置各种效果
✅ **玩家控制** - 按C键切换控制摄像机或物体

---

## 🚀 当前配置（engine.cpp）

```cpp
// 创建史莱姆
Slime* slime = new Slime(
    this,                           // Engine 指针
    glm::vec3(-2.0f, -3.0f, 0.0f), // 初始位置（中心点）
    280,                            // 粒子数量
    0.08f,                          // 粒子半径
    2.0f,                           // 初始生成半径（粒子分布范围）
    sphereShader,                   // 着色器
    texture2                        // 纹理
);

slime->setCohesionForce(1.35f);    // 向心力强度
slime->setDamping(0.83f);          // 水平阻尼系数
slime->setMaxCohesionDistance(2.5f); // 向心力作用距离
scene->addObject(slime);

// 绑定到玩家控制器
playerController->setControlledObject(slime);
playerController->setMoveSpeed(500.0f);
```

---

## 🎨 参数调节指南

### 1. 粒子数量 (numParticles)
```cpp
30-40  → 流畅性能，粗糙效果
50-70  → 平衡性能和效果 ✅ 推荐
80+    → 细腻效果，性能要求高
```

### 2. 粒子半径 (particleRadius)
```cpp
0.08   → 小粒子，细腻但易穿透
0.10   → 适中 ✅ 推荐
0.15   → 大粒子，明显但碰撞多
```

### 3. 向心力 (cohesionForce)
```cpp
1.0-3.0   → 松散，像水滴
4.0-6.0   → 适中，保持形状 ✅ 推荐
7.0-15.0  → 紧密，像果冻
```

**注意**：向心力公式已改为 `F = k / (distance + 0.1)`，距离越远力越小

### 4. 阻尼系数 (damping)
```cpp
0.95      → 高阻尼，快速停止
0.98      → 适中 ✅ 推荐
0.995     → 低阻尼，持续运动
```

**重要**：阻尼只影响水平速度，不影响垂直下落（重力正常工作）

### 5. 初始半径 (initialRadius)
```cpp
1.0   → 紧凑分布，可能无法生成所有粒子
1.5   → 适中 ✅ 推荐
2.0   → 松散分布，所有粒子不重叠
```

**提示**：如果控制台显示 "警告：只创建了 X / Y 个粒子"，说明空间不足，增大 initialRadius

---

## 🔧 物理属性（在 slime.cpp 中）

```cpp
// initParticles() 中的设置
friction = 0.05f        // 极低摩擦力，实现流动性
bounciness = 0.5f       // 中等弹性
linearDamping = 0.1f    // 物理引擎的空气阻力
massDensity = 1.0f      // 质量密度
```

---

## 📊 常见效果配置

### 水滴效果（分散、流动）
```cpp
Slime* slime = new Slime(this, glm::vec3(0.0f, 5.0f, 0.0f), 40, 0.12f, 1.5f, shader, tex);
slime->setCohesionForce(2.0f);
slime->setDamping(0.99f);
slime->setMaxCohesionDistance(2.0f);
```

### 果冻效果（弹性、晃动）
```cpp
Slime* slime = new Slime(this, glm::vec3(0.0f, 5.0f, 0.0f), 50, 0.10f, 1.0f, shader, tex);
slime->setCohesionForce(8.0f);
slime->setDamping(0.96f);
slime->setMaxCohesionDistance(1.5f);
```

### 黏稠效果（紧密、慢速）
```cpp
Slime* slime = new Slime(this, glm::vec3(0.0f, 5.0f, 0.0f), 60, 0.08f, 0.8f, shader, tex);
slime->setCohesionForce(12.0f);
slime->setDamping(0.93f);
slime->setMaxCohesionDistance(1.0f);
```

---

## ⚠️ 常见问题

### Q: 粒子飞散开了
**A**: 增加向心力或减小作用距离
```cpp
slime->setCohesionForce(10.0f);  // 增大向心力
slime->setMaxCohesionDistance(1.5f);  // 减小作用距离
```

### Q: 粒子粘成一团，不流动
**A**: 已修复！确保使用最新代码（摩擦力 = 0.05）

### Q: 控制台显示"只创建了 X 个粒子"
**A**: 初始半径太小，增大 initialRadius
```cpp
Slime* slime = new Slime(..., 2.0f, ...);  // 增大第5个参数
```

### Q: 控制史莱姆时移动太慢/太快
**A**: 调整移动力度
```cpp
// 史莱姆需要更大的力（因为有多个粒子）
playerController->setMoveSpeed(500.0f);  // 推荐 300-800

// 如果還是太慢，尝试 800-1000
playerController->setMoveSpeed(800.0f);
```

### Q: 按C键没反应
**A**: 确保已绑定控制对象
```cpp
playerController->setControlledObject(slime);
```

---

## 🧮 关键技术点

### 1. 统一的力施加接口
```cpp
// Object 基类定义纯虚函数
virtual void applyForce(const glm::vec3& force) = 0;

// 每个子类实现自己的逻辑：
// Cube/Sphere: 对单个刚体施加力
// Slime: 对所有粒子施加力
// Plane: 静态对象不响应力
```

### 2. 阻尼系统
```cpp
// 只对水平速度应用阻尼（不影响重力）
glm::vec3 horizontalVel(vel.x, 0.0f, vel.z);
horizontalVel *= m_damping;  // 只阻尼水平运动

// 垂直速度保持不变
glm::vec3 newVel(horizontalVel.x, vel.y, horizontalVel.z);
```

### 3. 向心力（改进版）
```cpp
// 距离越远，力越小（类似引力）
float forceMagnitude = cohesionForce / (distance + 0.1f);
glm::vec3 force = normalize(toCenter) * forceMagnitude;
```

### 4. 粒子初始化（防重叠）
```cpp
// 检查每个新粒子是否与已有粒子重叠
float minDistance = particleRadius * 2.1f;
if (distance(newPos, existingPos) < minDistance) {
    // 重新生成位置
}
```

### 5. 低摩擦力
```cpp
// 极低的摩擦力实现液态流动性
setFrictionCoefficient(0.05f);  // 而不是默认的 0.5
```

---

## 💡 性能优化建议

- 调试时：30-40 粒子
- 正式场景：50-70 粒子
- 不建议超过 100 粒子（除非性能足够）
- 每个场景 2-3 个史莱姆最佳

---

## 🎮 操作指南

| 按键 | 功能 |
|------|------|
| **C** | 切换控制模式（摄像机/物体）|
| **WASD** | 移动（相对摄像机方向）|
| **空格** | 向上移动/施加力 |
| **Shift** | 向下移动/施加力 |
| **鼠标** | 旋转视角 |
| **Alt** | 切换鼠标捕获 |
| **Z** | 摄像机看向史莱姆 |

---

## 📁 相关代码文件

```
engine/object/slime.h          - 史莱姆类定义
engine/object/slime.cpp        - 史莱姆类实现
engine/playerController.h      - 玩家控制器定义
engine/playerController.cpp    - 玩家控制器实现
engine/engine.cpp              - 创建史莱姆和绑定控制器
assets/shaders/slime_*.glsl    - 液态着色器（可选，未启用）
```

---

## 🎉 完成状态

✅ 粒子物理系统
✅ 碰撞检测
✅ 向心力（距离越远力越小）
✅ 流动性优化（低摩擦）
✅ 阻尼修复（不影响重力）
✅ 粒子初始化（防重叠）
✅ 玩家控制器（切换控制模式）
✅ 参数可调节

**当前渲染**：每个粒子作为球体渲染，可见物理效果
**未来扩展**：表面重建（Marching Cubes）生成光滑网格

---

**使用提示**：
- 按 **`Alt`** 键切换鼠标捕获
- 按 **`C`** 键切换控制模式
- 在物体控制模式下，WASD 会施加力移动史莱姆
- 摄像机方向决定移动方向（相对移动）
