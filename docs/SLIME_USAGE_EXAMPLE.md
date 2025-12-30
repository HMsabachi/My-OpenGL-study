# 🟢 史莱姆系统使用指南

## 📋 快速开始

史莱姆类已经创建完成并集成到项目中，使用基于物理的粒子系统模拟液态效果。

---

## 🎯 核心特性

✅ **物理模拟** - 每个粒子是独立的物理刚体
✅ **碰撞检测** - 粒子之间、粒子与场景物体自动碰撞
✅ **向心力系统** - 保持史莱姆形状的聚合力
✅ **流动性** - 低摩擦力实现液态效果
✅ **可调参数** - 灵活配置各种效果

---

## 🚀 当前配置（engine.cpp）

```cpp
// 创建史莱姆
Slime* slime = new Slime(
    this,                           // Engine 指针
    glm::vec3(0.0f, 5.0f, 0.0f),   // 初始位置（中心点）
    70,                             // 粒子数量
    0.1f,                           // 粒子半径
    1.0f,                           // 初始生成半径（粒子分布范围）
    sphereShader,                   // 着色器
    texture2                        // 纹理
);

slime->setCohesionForce(6.0f);    // 向心力强度
slime->setDamping(0.995f);         // 水平阻尼系数
scene->addObject(slime);
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

### 4. 阻尼系数 (damping)
```cpp
0.95      → 高阻尼，快速停止
0.98      → 适中 ✅ 推荐
0.995     → 低阻尼，持续运动
```

**重要**：阻尼只影响水平速度，不影响垂直下落（重力正常工作）

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
```

### 果冻效果（弹性、晃动）
```cpp
Slime* slime = new Slime(this, glm::vec3(0.0f, 5.0f, 0.0f), 50, 0.10f, 1.0f, shader, tex);
slime->setCohesionForce(8.0f);
slime->setDamping(0.96f);
```

### 黏稠效果（紧密、慢速）
```cpp
Slime* slime = new Slime(this, glm::vec3(0.0f, 5.0f, 0.0f), 60, 0.08f, 0.8f, shader, tex);
slime->setCohesionForce(12.0f);
slime->setDamping(0.93f);
```

---

## ⚠️ 常见问题

### Q: 粒子飞散开了
**A**: 增加向心力
```cpp
slime->setCohesionForce(10.0f);  // 从 6.0 增加到 10.0
```

### Q: 粒子粘成一团，不流动
**A**: 已修复！确保使用最新代码（摩擦力 = 0.05）

### Q: 下落太慢
**A**: 已修复！阻尼现在只影响水平速度，垂直下落不受影响

### Q: 粒子穿透地板
**A**: 确保地板有物理属性：
```cpp
floor->initPhysics(Object::PhysicsType::STATIC, 
                   Object::CollisionShape::PLANE, 
                   glm::vec3(50.0f, 0.2f, 50.0f));
```

---

## 🧮 关键技术点

### 1. 阻尼系统
```cpp
// 只对水平速度应用阻尼（不影响重力）
glm::vec3 horizontalVel(vel.x, 0.0f, vel.z);
horizontalVel *= m_damping;  // 只阻尼水平运动

// 垂直速度保持不变
glm::vec3 newVel(horizontalVel.x, vel.y, horizontalVel.z);
```

### 2. 向心力
```cpp
// 计算指向中心的力
glm::vec3 toCenter = center - position;
float distance = glm::length(toCenter);

// 力 = 向心力系数 × 距离（有最大作用距离限制）
glm::vec3 force = normalize(toCenter) * cohesionForce * distance;
```

### 3. 低摩擦力
```cpp
// 极低的摩擦力实现液态流动性
setFrictionCoefficient(0.05f);  // 而不是默认的 0.5
```

---

## 💡 性能优化建议

- 调试时：30-40 粒子
- 正式场景：50-70 粒子
- 不建议超过 100 粒子
- 每个场景 2-3 个史莱姆最佳

---

## 📁 相关代码文件

```
engine/object/slime.h         - 史莱姆类定义
engine/object/slime.cpp       - 史莱姆类实现
engine/engine.cpp             - 创建史莱姆的示例
assets/shaders/slime_*.glsl   - 液态着色器（可选，未启用）
```

---

## 🎉 完成状态

✅ 粒子物理系统
✅ 碰撞检测
✅ 向心力
✅ 流动性优化（低摩擦）
✅ 阻尼修复（不影响重力）
✅ 参数可调节

**当前渲染**：每个粒子作为球体渲染，可见物理效果
**未来扩展**：表面重建（Marching Cubes）生成光滑网格

---

**使用提示**：按 `Alt` 键切换鼠标捕获，WASD 移动相机，观察史莱姆的下落和聚合效果。
