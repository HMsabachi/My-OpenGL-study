# 🎯 PBF 史莱姆快速调参指南

## 📋 参数速查表

| 参数名 | 推荐值 | 影响 | 调大效果 | 调小效果 |
|--------|--------|------|----------|----------|
| `particleCount` | 100-500 | 流体精度 | 更平滑，更慢 | 更快，颗粒感强 |
| `particleRadius` | 0.08-0.12 m | 视觉大小 | 粒子更大 | 粒子更小 |
| `smoothingRadius` | 0.1-0.15 m | 邻居范围 | 更平滑，更慢 | 更快，分离感强 |
| `restDensity` | 6000-8000 kg/m³ | 密度目标 | 粒子间距大 | 粒子间距小 |
| `epsilon` | 300-1000 | 稳定性 | 更稳定，更软 | 不稳定，更硬 |
| `viscosity` | 0.01-0.05 | 粘稠度 | 更粘稠，慢 | 更流动，快 |
| `cohesion` | 0.005-0.02 | 向心力 | 更聚集 | 更分散 |
| `solverIterations` | 3-5 | 精度 | 更准确，更慢 | 更快，不稳定 |
| `boundaryDamping` | 0.7-0.9 | 碰撞反弹 | 反弹弱 | 反弹强 |
| `tensileK` | 0.0001-0.001 | 张力修正 | 分散感强 | 聚集感强 |

---

## 🎨 常见效果调参

### 1️⃣ **水一样的流体**
```cpp
params.viscosity = 0.01f;        // 低粘度
params.cohesion = 0.005f;        // 低向心力
params.surfaceTension = 0.01f;   // 高表面张力
params.boundaryDamping = 0.7f;   // 低阻尼（易反弹）
```

### 2️⃣ **蜂蜜般的粘稠液体**
```cpp
params.viscosity = 0.1f;         // 高粘度
params.cohesion = 0.02f;         // 高向心力
params.solverIterations = 5;     // 更多迭代
params.boundaryDamping = 0.9f;   // 高阻尼（不易反弹）
```

### 3️⃣ **Q弹的史莱姆**
```cpp
params.viscosity = 0.03f;        // 中等粘度
params.cohesion = 0.015f;        // 中高向心力
params.epsilon = 800.0f;         // 较软
params.boundaryDamping = 0.8f;   // 中等反弹
```

### 4️⃣ **爆炸效果的液体**
```cpp
params.cohesion = 0.001f;        // 极低向心力
params.viscosity = 0.005f;       // 极低粘度
params.boundaryDamping = 0.5f;   // 高反弹
// 在更新时施加强力：
mySlime->applyForce(glm::vec3(0.0f, 100.0f, 0.0f));
```

---

## ⚡ 性能优化参数

### 🐌 **太慢？试试这些：**
1. **减少粒子数量**
   ```cpp
   new Slime(..., 100, ...);  // 从 500 降到 100
   ```

2. **减小核半径**
   ```cpp
   params.smoothingRadius = 0.08f;  // 从 0.15 降到 0.08
   ```

3. **减少迭代次数**
   ```cpp
   params.solverIterations = 3;  // 从 5 降到 3
   ```

4. **禁用可选效果**
   ```cpp
   // 注释掉涡量约束
   // applyVorticityConfinement(dt);
   ```

### 🚀 **还有性能余量？提升质量：**
1. **增加粒子数量**
   ```cpp
   new Slime(..., 500, ...);  // 从 100 提升到 500
   ```

2. **增加迭代次数**
   ```cpp
   params.solverIterations = 5;  // 从 3 提升到 5
   ```

3. **增加子步长**
   ```cpp
   const int subSteps = 3;  // 从 2 提升到 3
   ```

---

## 🐛 常见问题排查

### ❌ **粒子飞散，无法聚集**
**原因：** 向心力太弱或核半径太小

**解决：**
```cpp
params.cohesion = 0.02f;         // 增大向心力
params.smoothingRadius = 0.12f;  // 增大核半径
```

---

### ❌ **粒子聚成一团，不流动**
**原因：** 向心力太强或粘度太高

**解决：**
```cpp
params.cohesion = 0.005f;   // 减小向心力
params.viscosity = 0.01f;   // 减小粘度
```

---

### ❌ **粒子穿透地面或墙壁**
**原因：** 时间步长太大或边界检测失效

**解决：**
```cpp
// 1. 限制时间步长
float dt = glm::clamp(deltaTime, 0.001f, 0.016f);

// 2. 增加子步长
const int subSteps = 3;

// 3. 检查地面碰撞代码
if (p.predictedPosition.y < groundY) {
    p.predictedPosition.y = groundY;
}
```

---

### ❌ **模拟不稳定，粒子爆炸**
**原因：** 密度计算错误或松弛系数太小

**解决：**
```cpp
// 1. 增大松弛系数
params.epsilon = 1000.0f;  // 从 600 提升到 1000

// 2. 检查密度是否合理
std::cout << "平均密度: " << avgDensity 
          << " (目标: " << params.restDensity << ")" << std::endl;

// 3. 调整密度值
params.restDensity = 6378.0f;  // 根据实际密度调整
```

---

### ❌ **与其他物体没有碰撞**
**原因：** 刚体类型错误或碰撞检测未实现

**解决：**
```cpp
// 1. 确认刚体为 KINEMATIC
m_rigidBody->setType(rp3d::BodyType::KINEMATIC);

// 2. 确认碰撞体已添加
if (!m_collider) {
    std::cerr << "❌ 碰撞体未创建！" << std::endl;
}

// 3. 实现碰撞响应
void Slime::interactWithPhysicsWorld() {
    // 检测刚体是否被推动
    glm::vec3 offset = physicsCenter - m_slimeCenter;
    if (glm::length(offset) > EPSILON) {
        // 应用偏移到所有粒子
        for (auto& p : m_particles) {
            p.position += offset;
        }
    }
}
```

---

## 🎮 实时调参技巧

### **方法1：键盘快捷键**
```cpp
void Engine::keyCallback(int key, int action, int mods) {
    if (action == GLFW_PRESS) {
        auto params = mySlime->getPBFParams();
        
        switch (key) {
            case GLFW_KEY_KP_ADD:  // 数字键盘 +
                params.cohesion += 0.001f;
                std::cout << "向心力: " << params.cohesion << std::endl;
                break;
            
            case GLFW_KEY_KP_SUBTRACT:  // 数字键盘 -
                params.cohesion -= 0.001f;
                std::cout << "向心力: " << params.cohesion << std::endl;
                break;
            
            case GLFW_KEY_UP:
                params.viscosity += 0.01f;
                std::cout << "粘度: " << params.viscosity << std::endl;
                break;
            
            case GLFW_KEY_DOWN:
                params.viscosity -= 0.01f;
                std::cout << "粘度: " << params.viscosity << std::endl;
                break;
        }
        
        mySlime->setPBFParams(params);
    }
}
```

### **方法2：ImGui 面板**
```cpp
void Engine::renderImGui() {
    ImGui::Begin("PBF 参数调节");
    
    auto params = mySlime->getPBFParams();
    
    ImGui::SliderFloat("粘度", &params.viscosity, 0.0f, 0.1f);
    ImGui::SliderFloat("向心力", &params.cohesion, 0.0f, 0.05f);
    ImGui::SliderFloat("核半径", &params.smoothingRadius, 0.05f, 0.2f);
    ImGui::SliderInt("迭代次数", &params.solverIterations, 1, 10);
    
    if (ImGui::Button("重置为水")) {
        params.viscosity = 0.01f;
        params.cohesion = 0.005f;
    }
    
    if (ImGui::Button("重置为蜂蜜")) {
        params.viscosity = 0.1f;
        params.cohesion = 0.02f;
    }
    
    mySlime->setPBFParams(params);
    
    ImGui::End();
}
```

---

## 📊 密度计算参考

### **粒子间距与密度的关系**
```
平均粒子间距 d ≈ 2 * radius / ∛(particleCount)

核半径 h ≈ 2 * d  (通常为 1.5~3 倍间距)

静止密度 ρ₀ = particleCount * particleMass / totalVolume
```

### **示例计算**
```cpp
// 场景参数
int particleCount = 200;
float boundaryRadius = 1.0f;  // m

// 计算平均间距
float avgDistance = 2.0f * boundaryRadius / std::cbrt(particleCount);
// avgDistance ≈ 0.34 m

// 设置核半径
float smoothingRadius = 2.0f * avgDistance;  // 0.068 m → 取 0.1 m

// 计算粒子质量
float particleRadius = 0.08f;  // m
float particleVolume = (4.0f / 3.0f) * PI * pow(particleRadius, 3.0f);
float totalVolume = (4.0f / 3.0f) * PI * pow(boundaryRadius, 3.0f);

// 假设总质量为 10 kg
float totalMass = 10.0f;
float particleMass = totalMass / particleCount;  // 0.05 kg

// 计算静止密度
float restDensity = particleCount * particleMass / totalVolume;
// restDensity ≈ 2387 kg/m³
```

---

## 🔬 调试输出

### **基本统计信息**
```cpp
void Slime::update(float deltaTime) {
    static int frameCount = 0;
    if (++frameCount % 60 == 0) {  // 每60帧打印一次
        float avgDensity = 0.0f;
        float avgSpeed = 0.0f;
        int avgNeighbors = 0;
        
        for (const auto& p : m_particles) {
            avgDensity += p.density;
            avgSpeed += glm::length(p.velocity);
        }
        
        for (const auto& neighbors : m_neighbors) {
            avgNeighbors += neighbors.size();
        }
        
        avgDensity /= m_particles.size();
        avgSpeed /= m_particles.size();
        avgNeighbors /= m_neighbors.size();
        
        std::cout << "============== 史莱姆状态 ==============" << std::endl;
        std::cout << "平均密度: " << avgDensity << " kg/m³ "
                  << "(目标: " << m_pbfParams.restDensity << ")" << std::endl;
        std::cout << "平均速度: " << avgSpeed << " m/s" << std::endl;
        std::cout << "平均邻居数: " << avgNeighbors << std::endl;
        std::cout << "质心位置: " << m_slimeCenter.x << ", " 
                  << m_slimeCenter.y << ", " << m_slimeCenter.z << std::endl;
        std::cout << "=======================================" << std::endl;
    }
}
```

---

## 🎯 最佳实践

1. **从少量粒子开始**：先用 50-100 个粒子测试参数，满意后再增加到 200-500
2. **逐步调整参数**：每次只改一个参数，观察效果
3. **保存配置**：找到满意的参数后保存到配置文件
4. **使用子步长**：2-3 个子步长可以大幅提高稳定性
5. **监控密度**：平均密度应接近 restDensity（误差 ±10% 内）

---

## 📞 快速联系

如果遇到问题，检查这些关键点：
- ✅ 密度计算是否合理（平均密度接近 restDensity）
- ✅ 邻居搜索是否正常（平均邻居数 10-30）
- ✅ 粒子是否在边界内（无穿透或飞散）
- ✅ 时间步长是否合理（≤ 0.016s）
- ✅ 刚体类型是否正确（KINEMATIC）

祝调参顺利！🎉
