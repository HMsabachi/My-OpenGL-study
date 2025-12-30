# Slime 类使用指南

## 📝 快速开始

### 1. 在 engine.cpp 中添加头文件

在 `engine/engine.cpp` 顶部添加：

```cpp
#include "object/slime.h" // 引入 Slime 类
```

### 2. 加载 Slime 的着色器

在 `Engine::init()` 函数中添加：

```cpp
shaderManager->loadShader("slime", "assets/shaders/slime_vertex.glsl", "assets/shaders/slime_fragment.glsl");
```

### 3. 在 setupDemoData 中创建 Slime

在 `Engine::setupDemoData()` 函数中添加：

```cpp
// 配置 slime shader
auto* slimeShader = shaderManager->getShader("slime");
slimeShader->begin();
slimeShader->setInt("texture1", 0);  // 纹理单元 0
slimeShader->end();

// 创建史莱姆对象
// 参数：引擎, 中心位置, 粒子数量, 粒子半径, 初始半径, 着色器, 纹理
Slime* slime = new Slime(
    this,                           // Engine 指针
    glm::vec3(0.0f, 3.0f, 0.0f),   // 中心位置（在空中）
    30,                             // 粒子数量
    0.12f,                          // 每个粒子的半径
    0.8f,                           // 初始生成半径
    slimeShader,                    // 着色器
    texture2                        // 纹理（可选）
);

// 设置史莱姆的物理参数（可选）
slime->setCohesionForce(15.0f);    // 向心力强度（默认 10.0）
slime->setDamping(0.95f);          // 阻尼系数（默认 0.98）

// 添加到场景
scene->addObject(slime);
```

### 4. 更新全局 Uniforms

在 `Engine::updateGlobalUniforms()` 函数末尾添加：

```cpp
auto* slimeShader = shaderManager->getShader("slime");
slimeShader->begin();
slimeShader->setMat4("uView", viewMatrix);
slimeShader->setMat4("uProjection", projectionMatrix);
// Slime 着色器需要额外的 uniforms
slimeShader->setVec3("uCameraPos", camera->getPosition());
slimeShader->setFloat("uTime", (float)glfwGetTime());
slimeShader->setVec3("uSlimeColor", glm::vec3(0.2f, 0.8f, 0.3f)); // 绿色史莱姆
slimeShader->end();
```

## 🎨 参数调整指南

### 粒子数量 (numParticles)
- **10-20**: 小型史莱姆，性能好
- **30-50**: 中型史莱姆，推荐
- **50-100**: 大型史莱姆，性能要求高

### 粒子半径 (particleRadius)
- **0.08-0.12**: 小粒子，细腻效果
- **0.12-0.20**: 中等粒子，推荐
- **0.20-0.30**: 大粒子，更明显

### 初始半径 (initialRadius)
- **0.5-1.0**: 紧凑的史莱姆
- **1.0-2.0**: 松散的史莱姆

### 向心力 (cohesionForce)
- **5.0-10.0**: 弱向心力，松散
- **10.0-20.0**: 中等向心力，推荐
- **20.0-50.0**: 强向心力，紧密

### 阻尼 (damping)
- **0.90-0.95**: 高阻尼，慢动作
- **0.95-0.98**: 中等阻尼，推荐
- **0.98-0.99**: 低阻尼，弹跳感强

## 🎯 效果预期

运行程序后，你应该看到：

1. ✅ 一团由小球体组成的史莱姆
2. ✅ 球体会受重力下落
3. ✅ 球体之间有碰撞效果
4. ✅ 球体会被向心力拉向中心
5. ✅ 落到地板后会形成"液态堆"

## 🔧 常见问题

### Q: 史莱姆粒子飞散开了
A: 增加 `cohesionForce` 的值，例如从 10.0 改为 20.0

### Q: 史莱姆太僵硬，不够液态
A: 减小 `damping` 的值，例如从 0.98 改为 0.92

### Q: 史莱姆穿透地板
A: 确保地板已经设置了物理属性：
```cpp
floor->initPhysics(Object::PhysicsType::STATIC, Object::CollisionShape::PLANE, glm::vec3(50.0f, 0.2f, 50.0f));
```

### Q: 粒子太小或太大
A: 调整 `particleRadius` 参数

## 📊 性能建议

- 在调试时使用较少的粒子（20-30）
- 正式场景可以使用更多粒子（50-80）
- 每个场景建议不超过 2-3 个史莱姆对象
- 粒子总数建议控制在 200 以内

## 🚀 下一步优化方向

1. **表面重建**：使用 Marching Cubes 算法生成光滑的史莱姆表面
2. **颜色变化**：根据压力或速度改变史莱姆颜色
3. **分裂合并**：实现史莱姆的分裂和合并效果
4. **交互**：添加玩家可以"戳"史莱姆的功能
