# 🌊 PBF (Position Based Fluids) 物理模拟修复文档

## 📋 问题诊断

你的史莱姆液体模拟存在以下关键问题：

### 1. **PBF 算法实现不完整**
- ❌ 缺少张力修正（Tensile Instability Correction）
- ❌ 位置修正没有累积，每次迭代都覆盖
- ❌ 核函数系数没有预计算，性能低下
- ❌ 缺少子步长模拟，导致大时间步长不稳定

### 2. **物理参数设置不合理**
- ❌ 密度值 `1000 kg/m³` 与粒子间距不匹配
- ❌ 向心力过强 (`0.05`)，导致粒子聚团
- ❌ 核半径 `0.3m` 过大，导致邻居搜索开销大
- ❌ 松弛系数过小，数值不稳定

### 3. **与 ReactPhysics3D 交互不正确**
- ❌ 史莱姆整体使用 `DYNAMIC` 刚体，受重力影响整体下落
- ❌ 粒子没有与环境物体碰撞检测
- ❌ 外部力没有正确传递到粒子系统

---

## ✅ 解决方案

### 1. 完善 PBF 算法（基于 Macklin & Müller 2013 论文）

#### **标准 PBF 流程**
```cpp
void Slime::updatePBF(float dt) {
    // 1. 应用外力 (重力 + 用户输入)
    applyExternalForces(dt);
    
    // 2. 预测位置 x* = x + Δt·v
    predictPositions(dt);
    
    // 3. 查找邻居 (空间哈希加速)
    findNeighbors();
    
    // 4. 求解器循环
    resetDeltaP();  // ✅ 重置位置修正累积
    for (int iter = 0; iter < solverIterations; ++iter) {
        // 4a. 计算密度和 lambda
        computeDensityAndLambda();
        
        // 4b. 计算位置修正 Δp
        computePositionCorrection();  // ✅ 累积到 deltaP
        
        // 4c. 应用向心力 (最后一次迭代)
        if (iter == solverIterations - 1) {
            applyCohesionForce();
        }
    }
    
    // 5. 应用位置修正 x* = x* + Δp
    applyPositionCorrection();
    
    // 6. 碰撞检测与响应
    handleCollisions();
    
    // 7. 更新速度 v = (x* - x) / Δt
    updateVelocities(dt);
    
    // 8. XSPH 粘度 (平滑速度场)
    applyXSPHViscosity();
}
```

#### **关键改进点**

##### ✅ 1. 位置修正累积
```cpp
// 错误做法：每次迭代覆盖
void computePositionCorrection() {
    for (size_t i = 0; i < m_particles.size(); ++i) {
        m_particles[i].predictedPosition += deltaPi;  // ❌ 直接修改
    }
}

// 正确做法：累积后统一应用
void computePositionCorrection() {
    for (size_t i = 0; i < m_particles.size(); ++i) {
        m_particles[i].deltaP += deltaPi;  // ✅ 累积
    }
}

void applyPositionCorrection() {
    for (auto& p : m_particles) {
        p.predictedPosition += p.deltaP;  // ✅ 统一应用
    }
}
```

##### ✅ 2. 张力修正（防止粒子聚团）
```cpp
void computePositionCorrection() {
    for (size_t i = 0; i < m_particles.size(); ++i) {
        for (int j : m_neighbors[i]) {
            float lambdaSum = m_particles[i].lambda + m_particles[j].lambda;
            
            // ✅ 张力修正 s_corr = -k * (W_ij / W_deltaQ)^n
            float wij = poly6Kernel(glm::length(r));
            float sCorr = -tensileK * pow(wij / wDeltaQPow, tensileN);
            
            deltaPi += (lambdaSum + sCorr) * spikyGradient(r) / rho0;
        }
    }
}
```

**参数说明：**
- `deltaQ = 0.3 * h`：张力修正参考距离（通常为 0.1~0.3 倍核半径）
- `tensileK = 0.0001`：张力强度
- `tensileN = 4`：幂次

##### ✅ 3. XSPH 粘度（平滑速度场）
```cpp
void applyXSPHViscosity() {
    for (size_t i = 0; i < m_particles.size(); ++i) {
        glm::vec3 velocityCorrection(0.0f);
        
        for (int j : m_neighbors[i]) {
            float r = glm::length(m_particles[i].position - m_particles[j].position);
            glm::vec3 vij = m_particles[j].velocity - m_particles[i].velocity;
            velocityCorrection += vij * poly6Kernel(r);
        }
        
        m_particles[i].velocity += viscosity * velocityCorrection;
    }
}
```

##### ✅ 4. 子步长模拟（提高稳定性）
```cpp
void Slime::update(float deltaTime) {
    float dt = glm::clamp(deltaTime, 0.001f, 0.016f);
    
    // ✅ 子步长模拟：将大时间步长分割为多个小步
    const int subSteps = 2;
    float subDt = dt / static_cast<float>(subSteps);
    
    for (int i = 0; i < subSteps; ++i) {
        updatePBF(subDt);
    }
}
```

**优点：**
- 提高数值稳定性
- 减少能量损失
- 防止粒子穿透

---

### 2. 合理的物理参数设置

#### **密度计算公式**
根据 SPH 理论，静止密度 `ρ₀` 应满足：
```
ρ₀ = m / V_particle
```
其中：
- `m`：粒子质量
- `V_particle = (4/3) * π * r³`：粒子体积

**实际参数计算：**
```cpp
// 粒子参数
int particleCount = 200;
float particleRadius = 0.08f;  // m
float smoothingRadius = 0.1f;  // m (核半径，通常为 1~2 倍粒子半径)

// 计算粒子体积
float particleVolume = (4.0f / 3.0f) * PI * pow(particleRadius, 3.0f);

// 计算粒子质量（假设密度 ρ₀ = 6378 kg/m³）
float particleMass = restDensity * particleVolume / particleCount;
```

#### **推荐参数表**

| 参数 | 值 | 说明 |
|------|-----|------|
| `restDensity` | 6378 kg/m³ | 静止密度 (根据粒子间距调整) |
| `epsilon` | 600 | CFM 松弛参数 (数值稳定性) |
| `viscosity` | 0.02 | XSPH 粘度系数 |
| `cohesion` | 0.01 | 向心力强度 (史莱姆特性) |
| `smoothingRadius` | 0.1 m | SPH 核半径 |
| `solverIterations` | 4 | PBF 求解器迭代次数 |
| `boundaryDamping` | 0.8 | 边界碰撞阻尼 |
| `tensileK` | 0.0001 | 张力修正强度 |
| `tensileN` | 4 | 张力修正幂次 |

---

### 3. 与 ReactPhysics3D 的正确交互

#### **设计思路**
1. **史莱姆整体用 KINEMATIC 刚体**：代表质心，不受物理引擎重力影响
2. **粒子自己处理物理**：PBF 算法模拟流体行为
3. **刚体用于碰撞检测**：与其他物体（Cube、Sphere）交互

#### **实现细节**

##### ✅ 1. 初始化刚体
```cpp
Slime::Slime(...) {
    // 计算总质量
    float totalMass = m_particleMass * particleCount;
    
    // ✅ 使用 KINEMATIC 刚体（运动学体，不受物理引擎控制）
    initPhysics(PhysicsType::KINEMATIC, CollisionShape::SPHERE, 
                glm::vec3(radius), totalMass);
    
    // 禁用重力（粒子自己处理重力）
    if (m_rigidBody) {
        m_rigidBody->setType(rp3d::BodyType::KINEMATIC);
        m_rigidBody->enableGravity(false);
    }
}
```

##### ✅ 2. 同步位置到物理引擎
```cpp
void Slime::update(float deltaTime) {
    // 1. 更新 PBF 模拟
    updatePBF(deltaTime);
    
    // 2. 计算新的质心
    m_slimeCenter = getSlimeCenter();
    setPosition(m_slimeCenter);
    
    // 3. 同步到物理引擎刚体
    syncToPhysics();  // ✅ 更新刚体位置到质心
    
    // 4. 检测碰撞并响应
    interactWithPhysicsWorld();
}
```

##### ✅ 3. 碰撞检测
```cpp
void Slime::interactWithPhysicsWorld() {
    if (!m_rigidBody) return;
    
    // 如果刚体被其他物体推动，更新粒子位置
    const rp3d::Transform& transform = m_rigidBody->getTransform();
    glm::vec3 physicsCenter = convertFromRP3D(transform.getPosition());
    
    glm::vec3 offset = physicsCenter - m_slimeCenter;
    
    if (glm::length(offset) > EPSILON) {
        // ✅ 刚体被推动，所有粒子跟随移动
        for (auto& p : m_particles) {
            p.position += offset;
        }
        m_slimeCenter = physicsCenter;
    }
}
```

##### ✅ 4. 环境碰撞（地面、墙壁）
```cpp
void Slime::handleCollisions() {
    // 1. 球形边界约束（史莱姆内部边界）
    for (auto& p : m_particles) {
        glm::vec3 toCenter = p.predictedPosition - m_slimeCenter;
        float dist = glm::length(toCenter);
        
        if (dist > m_boundaryRadius) {
            glm::vec3 normal = glm::normalize(toCenter);
            p.predictedPosition = m_slimeCenter + normal * m_boundaryRadius;
            
            // 速度反弹
            float vn = glm::dot(p.velocity, normal);
            if (vn > 0.0f) {
                p.velocity -= (1.0f + boundaryDamping) * vn * normal;
            }
        }
    }
    
    // 2. 环境碰撞（地面）
    for (auto& p : m_particles) {
        if (p.predictedPosition.y < -5.0f) {  // 地面 y = -5
            p.predictedPosition.y = -5.0f;
            
            if (p.velocity.y < 0.0f) {
                p.velocity.y *= -boundaryDamping;
                p.velocity.x *= 0.9f;  // 摩擦力
                p.velocity.z *= 0.9f;
            }
        }
    }
}
```

##### ✅ 5. 外部力传递
```cpp
void Slime::applyForce(const glm::vec3& force) {
    // 累积外部力（来自玩家输入或其他物体碰撞）
    m_externalForce += force;
    
    // 同时应用到物理刚体
    if (m_rigidBody) {
        rp3d::Vector3 rp3dForce(force.x, force.y, force.z);
        m_rigidBody->applyWorldForceAtCenterOfMass(rp3dForce);
    }
}

void Slime::applyExternalForces(float dt) {
    glm::vec3 gravity(0.0f, -9.81f, 0.0f);
    
    for (auto& p : m_particles) {
        // ✅ 重力 + 外部力
        p.velocity += (gravity + m_externalForce / m_particleMass) * dt;
    }
    
    // 清空外部力累积
    m_externalForce = glm::vec3(0.0f);
}
```

---

## 🎮 使用方法

### **1. 控制史莱姆**
```cpp
// 按 C 键切换控制模式
playerController->toggleControlMode();

// 在物体控制模式下：
// - WASD：移动史莱姆
// - Space：向上施加力
// - Shift：向下施加力
```

### **2. 调整参数**
```cpp
// 获取当前参数
Slime::PBFParams params = mySlime->getPBFParams();

// 修改参数
params.cohesion = 0.02f;         // 增加向心力，保持形状
params.viscosity = 0.05f;        // 增加粘度，减少混乱运动
params.smoothingRadius = 0.15f;  // 增大核半径，更平滑

// 应用参数
mySlime->setPBFParams(params);
```

### **3. 性能优化**
```cpp
// 减少粒子数量
Slime* mySlime = new Slime(this, center, radius, 100, shader, 0);  // 100 粒子

// 减少求解器迭代次数
params.solverIterations = 3;  // 从 4 降到 3

// 减小核半径（减少邻居搜索开销）
params.smoothingRadius = 0.08f;
```

---

## 📊 预期效果

### **改进前**
- ❌ 粒子飞散，无法保持形状
- ❌ 与地面碰撞后穿透
- ❌ 与其他物体没有交互
- ❌ 模拟不稳定，容易爆炸

### **改进后**
- ✅ 粒子保持球形聚集（向心力）
- ✅ 流体感明显（粘度 + XSPH）
- ✅ 与地面正确碰撞反弹
- ✅ 与其他物体（Cube、Sphere）碰撞时被推动
- ✅ 稳定运行，无穿透或爆炸

---

## 🔬 调试技巧

### **1. 可视化调试**
```cpp
// 在 render() 中绘制邻居连线
for (size_t i = 0; i < m_particles.size(); ++i) {
    for (int j : m_neighbors[i]) {
        drawLine(m_particles[i].position, m_particles[j].position, 
                 glm::vec3(0.0f, 1.0f, 0.0f));
    }
}

// 绘制速度向量
for (const auto& p : m_particles) {
    drawArrow(p.position, p.position + p.velocity * 0.1f, 
              glm::vec3(1.0f, 0.0f, 0.0f));
}
```

### **2. 控制台输出**
```cpp
// 每秒打印一次统计信息
static float timer = 0.0f;
timer += deltaTime;
if (timer >= 1.0f) {
    float avgNeighbors = 0.0f;
    float avgDensity = 0.0f;
    
    for (const auto& p : m_particles) {
        avgDensity += p.density;
    }
    avgDensity /= m_particles.size();
    
    std::cout << "平均密度: " << avgDensity 
              << " (目标: " << m_pbfParams.restDensity << ")" << std::endl;
    
    timer = 0.0f;
}
```

### **3. 检查常见问题**
```cpp
// 检查 NaN 或 Inf
for (const auto& p : m_particles) {
    if (std::isnan(p.position.x) || std::isinf(p.position.x)) {
        std::cerr << "❌ 粒子位置异常！" << std::endl;
    }
}

// 检查能量守恒
float totalEnergy = 0.0f;
for (const auto& p : m_particles) {
    float kinetic = 0.5f * p.mass * glm::dot(p.velocity, p.velocity);
    float potential = p.mass * 9.81f * p.position.y;
    totalEnergy += kinetic + potential;
}
std::cout << "总能量: " << totalEnergy << std::endl;
```

---

## 📚 参考资料

1. **PBF 原始论文**  
   Macklin, M., & Müller, M. (2013). Position based fluids. ACM Transactions on Graphics (TOG), 32(4), 1-12.  
   [论文链接](https://mmacklin.com/pbf_sig_preprint.pdf)

2. **ReactPhysics3D 文档**  
   [https://www.reactphysics3d.com/documentation/](https://www.reactphysics3d.com/documentation/)
   - Rigid Body API: 创建刚体、应用力
   - Collision Detection: 碰撞检测与查询

3. **SPH 核函数**  
   - Poly6 核：密度计算
   - Spiky 梯度核：压力计算
   - Viscosity 拉普拉斯核：粘度计算

4. **Position Based Dynamics**  
   Müller, M., et al. (2007). Position based dynamics. Journal of Visual Communication and Image Representation, 18(2), 109-118.

---

## 🎯 后续改进方向

### **1. 表面重建**
- 使用 Marching Cubes 算法生成平滑表面
- 替代当前的粒子球体渲染

### **2. 更复杂的碰撞检测**
```cpp
// 使用 ReactPhysics3D 的射线检测
rp3d::Ray ray(start, direction);
rp3d::RaycastInfo raycastInfo;
if (pWorld->raycast(ray, &raycastInfo)) {
    // 处理碰撞
}
```

### **3. 多相流体**
- 不同密度的粒子（水 + 油）
- 不同颜色和物理属性

### **4. GPU 加速**
- 使用 Compute Shader 计算邻居和密度
- 实时处理上万粒子

---

## ✨ 总结

本次修复的核心要点：

1. **完整的 PBF 算法实现**：包括张力修正、XSPH 粘度、子步长模拟
2. **合理的物理参数**：密度、核半径、向心力匹配
3. **与 ReactPhysics3D 的正确集成**：KINEMATIC 刚体 + 碰撞检测

通过这些改进，你的史莱姆现在应该能够：
- ✅ 稳定模拟流体行为
- ✅ 保持整体形状
- ✅ 与环境和其他物体正确交互

祝你在流体模拟的道路上越走越远！🎉
