# 史莱姆类 (Slime) - PBF 流体模拟实现文档

## 📋 概述

本文档详细说明了 `Slime` 类的实现，该类使用 **Position Based Fluids (PBF)** 算法模拟液态史莱姆的物理行为，并通过 ReactPhysics3D 与其他物理对象交互。

## 🎯 核心特性

### 1. **PBF 流体模拟**
- 基于 Macklin and Müller (2013) 的 Position Based Fluids 论文
- 粒子系统模拟液态行为
- 密度约束求解器
- SPH (Smoothed Particle Hydrodynamics) 核函数

### 2. **史莱姆特性**
- **向心力 (Cohesion Force)**: 使粒子聚集到质心，保持史莱姆形状
- **球形边界约束**: 限制粒子在一定范围内
- **粘度**: 模拟液体的黏稠特性
- **表面张力**: 保持液体表面完整性

### 3. **物理引擎集成**
- 使用 ReactPhysics3D 刚体作为整体碰撞体
- 自动同步粒子系统与物理引擎
- 支持与其他物体的碰撞和交互

### 4. **高效渲染**
- 实例化渲染 (Instanced Rendering) 绘制所有粒子
- 每个粒子渲染为小球体
- 单次 Draw Call 绘制数百个粒子

## 🔬 PBF 算法详解

### 算法流程

```
每一帧:
1. applyExternalForces(dt)      - 应用外力（重力）
2. predictPositions(dt)          - 预测下一帧位置
3. findNeighbors()               - 空间哈希查找邻居粒子
4. for iter in solverIterations:
   a. computeDensityAndLambda()  - 计算密度和拉格朗日乘数
   b. computePositionCorrection() - 计算位置修正
   c. applyCohesionForce()       - 应用向心力（史莱姆特性）
5. updateVelocities(dt)          - 从位置差更新速度
6. applyViscosityAndVorticity()  - 应用粘度和涡量约束
7. applyBoundaryConstraints()    - 边界碰撞处理
```

### 关键参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `restDensity` | 1000.0 | 静止密度 (kg/m³) |
| `relaxation` | 600.0 | 松弛系数（CFM约束力混合） |
| `viscosity` | 0.01 | 粘度系数 |
| `cohesion` | 0.05 | 向心力强度 |
| `smoothingRadius` | 0.3 | SPH 核半径 (m) |
| `solverIterations` | 4 | 密度约束求解迭代次数 |
| `boundaryDamping` | 0.5 | 边界碰撞能量损失 |

### SPH 核函数

#### 1. **Poly6 核** (密度计算)
```cpp
float poly6(float r, float h) {
    if (r >= h) return 0;
    float x = h² - r²;
    return (315 / (64π·h⁹)) · x³
}
```

#### 2. **Spiky 梯度核** (压力计算)
```cpp
vec3 spikyGradient(vec3 r, float h) {
    float rLen = length(r);
    if (rLen >= h) return vec3(0);
    float x = h - rLen;
    return (−45 / (π·h⁶)) · x² · (r / rLen)
}
```

#### 3. **粘度拉普拉斯核** (粘度计算)
```cpp
float viscosityLaplacian(float r, float h) {
    if (r >= h) return 0;
    return (45 / (π·h⁶)) · (h - r)
}
```

## 🔧 代码架构

### 类结构

```cpp
class Slime : public Object {
    // 粒子结构体
    struct Particle {
        glm::vec3 position;
        glm::vec3 predictedPosition;
        glm::vec3 velocity;
        glm::vec3 force;
        float density;
        float lambda;
    };
    
    // 粒子数据
    std::vector<Particle> m_particles;
    std::vector<std::vector<int>> m_neighbors;
    
    // 空间哈希（邻居搜索优化）
    std::unordered_map<int, std::vector<int>> m_spatialHash;
    
    // PBF 参数
    PBFParams m_pbfParams;
    
    // 渲染数据
    VAO* m_vao;
    std::shared_ptr<Buffer<float>> m_instanceVBO;
};
```

### 空间哈希算法

为了高效查找邻居粒子（在 `smoothingRadius` 范围内），使用空间哈希网格：

```cpp
int hashPosition(vec3 pos) {
    int x = floor(pos.x / cellSize);
    int y = floor(pos.y / cellSize);
    int z = floor(pos.z / cellSize);
    return (x * 73856093) ^ (y * 19349663) ^ (z * 83492791);
}
```

**时间复杂度**: O(n) 平均情况，相比暴力 O(n²) 有显著提升

### 实例化渲染

使用 OpenGL 实例化渲染，每个粒子共享一个球体网格，但有独立的变换矩阵：

```cpp
// 设置实例化属性 (Model Matrix per instance)
for (int i = 0; i < 4; ++i) {  // mat4 = 4 x vec4
    glEnableVertexAttribArray(3 + i);
    glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, 
                          sizeof(mat4), (void*)(sizeof(vec4) * i));
    glVertexAttribDivisor(3 + i, 1);  // 每实例更新一次
}

// 绘制
glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 
                        nullptr, particleCount);
```

## 🔄 物理引擎交互

### 集成策略

史莱姆同时使用两种物理表示：

1. **粒子系统** (内部): PBF 模拟液态行为
2. **刚体** (ReactPhysics3D): 与其他物体碰撞

### 同步机制

```cpp
void Slime::interactWithPhysicsWorld() {
    // 从物理引擎获取刚体位置
    vec3 physicsCenter = getRigidBody()->getPosition();
    
    // 计算偏移
    vec3 offset = physicsCenter - m_slimeCenter;
    
    // 应用到所有粒子（部分应用，避免震荡）
    for (auto& p : m_particles) {
        p.position += offset * 0.5f;
    }
}
```

### 碰撞响应

当史莱姆碰到其他物体时：
- ReactPhysics3D 处理刚体碰撞
- 粒子通过 `syncFromPhysics` 获取位移
- PBF 约束确保粒子不会穿透边界

## 🎨 使用示例

### 基本用法

```cpp
// 创建史莱姆（中心位置，半径，粒子数量）
Slime* slime = new Slime(
    engine,                          // 引擎指针
    glm::vec3(-5.0f, 2.0f, 0.0f),   // 初始位置
    1.5f,                            // 边界半径
    500,                             // 粒子数量
    shader,                          // Shader
    texture                          // 纹理
);

// 添加到场景
scene->addObject(slime);

// 绑定到玩家控制
playerController->setControlledObject(slime);
```

### 自定义参数

```cpp
// 调整 PBF 参数
Slime::PBFParams params;
params.restDensity = 1200.0f;      // 更高密度
params.cohesion = 0.1f;            // 更强向心力
params.viscosity = 0.02f;          // 更黏稠
params.solverIterations = 6;       // 更稳定（更慢）

slime->setPBFParams(params);
```

### 施加外力

```cpp
// 向上施加力（跳跃）
glm::vec3 jumpForce(0.0f, 1000.0f, 0.0f);
slime->applyForce(jumpForce);

// 力会均匀分配到所有粒子
```

## 📊 性能考虑

### 优化策略

1. **空间哈希**: O(n) 邻居搜索，避免 O(n²)
2. **实例化渲染**: 单次 Draw Call
3. **时间步限制**: 最大 dt = 0.016s，保证稳定性
4. **迭代次数**: 4次迭代平衡质量和性能

### 性能指标

| 粒子数量 | FPS (估计) | 邻居搜索时间 |
|----------|-----------|-------------|
| 100      | 60+       | ~0.1ms      |
| 500      | 60        | ~0.5ms      |
| 1000     | 30-60     | ~1.2ms      |
| 2000     | 15-30     | ~3ms        |

**建议**: 对于实时应用，保持粒子数量在 500-1000 之间

## 🐛 调试技巧

### 可视化调试

```cpp
// 在 render() 中绘制邻居连线
void Slime::renderDebug() {
    for (size_t i = 0; i < m_particles.size(); ++i) {
        for (int j : m_neighbors[i]) {
            drawLine(m_particles[i].position, 
                     m_particles[j].position);
        }
    }
}
```

### 常见问题

#### 1. **粒子爆炸**
- **原因**: 时间步太大或密度约束不足
- **解决**: 降低 `dt`，增加 `solverIterations`

#### 2. **粒子穿透边界**
- **原因**: 边界约束不够强
- **解决**: 增加 `boundaryDamping`，减小粒子速度上限

#### 3. **史莱姆散开**
- **原因**: 向心力太弱
- **解决**: 增加 `cohesion` 参数

#### 4. **性能下降**
- **原因**: 粒子数量太多或 `smoothingRadius` 太大
- **解决**: 减少粒子数量，优化空间哈希网格大小

## 📚 参考资料

### 核心论文

1. **Macklin, M., & Müller, M. (2013)**
   - *Position Based Fluids*
   - SIGGRAPH 2013
   - 📄 [PDF](https://mmacklin.com/pbf_sig_preprint.pdf)

2. **Müller, M., Charypar, D., & Gross, M. (2003)**
   - *Particle-Based Fluid Simulation for Interactive Applications*
   - Symposium on Computer Animation
   - 📄 [PDF](https://matthias-research.github.io/pages/publications/sca03.pdf)

### 推荐阅读

- [ReactPhysics3D Documentation](https://www.reactphysics3d.com/documentation/)
- [GPU Gems 3 - Chapter 30: Real-Time Simulation and Rendering of 3D Fluids](https://developer.nvidia.com/gpugems/gpugems3/part-v-physics-simulation/chapter-30-real-time-simulation-and-rendering-3d-fluids)
- [Fluid Engine Development](https://fluidenginedevelopment.org/)

## 🔮 未来扩展

### 计划功能

- [ ] **表面重建**: Marching Cubes 生成光滑表面
- [ ] **GPU 加速**: Compute Shader 加速 PBF 计算
- [ ] **多相流**: 支持不同密度的液体混合
- [ ] **双向耦合**: 粒子对刚体施加反作用力
- [ ] **自适应粒子**: 动态增删粒子优化性能

### 高级技术

```cpp
// GPU 计算示例 (Compute Shader)
#version 430 core

layout(local_size_x = 256) in;

struct Particle {
    vec3 position;
    vec3 velocity;
    float density;
};

layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

void main() {
    uint id = gl_GlobalInvocationID.x;
    // PBF 计算...
}
```

## 📝 总结

`Slime` 类展示了如何将现代流体模拟算法 (PBF) 与传统刚体物理引擎 (ReactPhysics3D) 结合，创造出既有流体特性又能与场景交互的对象。通过向心力等特殊约束，赋予了史莱姆独特的"保形"行为，区别于普通液体。

**核心优势**:
✅ 实时性能  
✅ 物理真实性  
✅ 易于扩展  
✅ 与现有系统无缝集成  

**适用场景**:
- 游戏中的史莱姆敌人
- 流体交互演示
- 物理模拟教学
- 特效粒子系统
