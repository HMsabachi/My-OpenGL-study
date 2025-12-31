# 史莱姆粒子渲染问题修复指南

## 🔍 问题诊断

如果你看不到史莱姆粒子，按以下步骤检查：

### 1. 检查粒子是否被创建

在 `Slime::initParticles()` 中添加调试输出：

```cpp
void Slime::initParticles(const glm::vec3& center, float radius, int count) {
    // ...初始化代码...
    
    std::cout << "✅ 创建了 " << m_particles.size() << " 个粒子" << std::endl;
    std::cout << "   中心位置: (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;
}
```

### 2. 检查渲染是否被调用

在 `Slime::render()` 中添加：

```cpp
void Slime::render() const {
    std::cout << "渲染史莱姆: " << m_particles.size() << " 个粒子" << std::endl;
    
    // ...渲染代码...
}
```

### 3. 检查粒子位置

在 `Slime::update()` 中打印第一个粒子的位置：

```cpp
void Slime::update(float deltaTime) {
    // ...更新代码...
    
    if (!m_particles.empty()) {
        auto& p = m_particles[0];
        std::cout << "粒子[0] 位置: (" << p.position.x << ", " << p.position.y << ", " << p.position.z << ")" << std::endl;
    }
}
```

### 4. 检查相机视角

确保相机能看到史莱姆：

```cpp
// 在 engine.cpp 的 update() 中
std::cout << "相机位置: (" << camera->getPosition().x << ", " 
          << camera->getPosition().y << ", " << camera->getPosition().z << ")" << std::endl;
std::cout << "史莱姆位置: (-5, 2, 0)" << std::endl;
```

## ✅ 修复方案（已实施）

我已经修复了以下问题：

### 问题 1: Shader 不支持实例化渲染 ✅

**原因**: 原来的 `sphere_vertex.glsl` 使用 `uniform mat4 uModel`，但实例化渲染需要从顶点属性读取每个粒子的矩阵。

**修复**: 更新了 `slime_vertex.glsl`：

```glsl
// 实例化属性 - 每个粒子的模型矩阵
layout (location = 3) in vec4 aInstanceMatrix0;
layout (location = 4) in vec4 aInstanceMatrix1;
layout (location = 5) in vec4 aInstanceMatrix2;
layout (location = 6) in vec4 aInstanceMatrix3;

void main()
{
    // 构建实例化矩阵
    mat4 instanceMatrix = mat4(
        aInstanceMatrix0,
        aInstanceMatrix1,
        aInstanceMatrix2,
        aInstanceMatrix3
    );
    
    // 使用实例化矩阵变换
    FragPos = vec3(instanceMatrix * vec4(animatedPos, 1.0));
    // ...
}
```

### 问题 2: Slime 渲染时设置了 uModel ✅

**原因**: `Slime::render()` 中设置了 `m_shader->setMat4("uModel", model)`，这会让所有粒子使用同一个矩阵，导致它们渲染到同一位置（重叠）。

**修复**: 移除了 `uModel` 设置：

```cpp
void Slime::render() const {
    m_shader->begin();
    
    // 启用混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 实例化绘制 - 不设置 uModel
    m_vao->drawInstanced(m_particles.size(), m_indexCount);
    
    glDisable(GL_BLEND);
    m_shader->end();
}
```

### 问题 3: 使用了错误的 Shader ✅

**原因**: `setupDemoData()` 中使用了 `sphereShader`，但它不支持实例化。

**修复**: 加载并使用专用的 `slimeShader`：

```cpp
// engine.cpp
shaderManager->loadShader("slime", 
    "assets/shaders/slime_vertex.glsl", 
    "assets/shaders/slime_fragment.glsl");

// setupDemoData()
auto* slimeShader = shaderManager->getShader("slime");
Slime* slime = new Slime(this, glm::vec3(-5.0f, 2.0f, 0.0f), 
                         1.5f, 500, slimeShader, 0);
```

## 🎮 控制指南

### 切换控制模式

1. **按 Alt** - 开启/关闭鼠标捕获
2. **按 C** - 切换相机模式/物体模式

### 相机模式 (默认)

- **WASD** - 移动相机
- **Space** - 向上
- **Shift** - 向下
- **鼠标** - 旋转视角

### 物体模式

- **WASD** - 移动史莱姆
- **Space** - 史莱姆跳跃
- **鼠标** - 旋转视角（相机不动）

### 快速找到史莱姆

按 **Z** 键让相机看向 `(-2, -5, 0)` 方向（靠近史莱姆初始位置 `(-5, 2, 0)`）

## 🔧 进一步调试

### 使用 RenderDoc 调试

1. 下载并安装 [RenderDoc](https://renderdoc.org/)
2. 启动 RenderDoc，加载你的程序
3. 捕获一帧
4. 检查：
   - Draw Call 数量（应该有一个实例化绘制调用）
   - 顶点缓冲内容
   - Shader 输入/输出

### 使用简单着色器测试

临时替换 `slime_fragment.glsl` 为纯色：

```glsl
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);  // 纯红色
}
```

如果这样能看到粒子，说明问题在 fragment shader 的光照计算。

### 检查 OpenGL 错误

在 `Slime::render()` 前后添加：

```cpp
GLenum err;
while ((err = glGetError()) != GL_NO_ERROR) {
    std::cerr << "OpenGL Error before render: " << err << std::endl;
}

// ...渲染代码...

while ((err = glGetError()) != GL_NO_ERROR) {
    std::cerr << "OpenGL Error after render: " << err << std::endl;
}
```

## 📊 性能监控

### 添加 FPS 计数器

在 `Engine::render()` 中：

```cpp
void Engine::render()
{
    static int frameCount = 0;
    static double lastTime = glfwGetTime();
    
    while (myApp->update()) {
        frameCount++;
        double currentTime = glfwGetTime();
        
        if (currentTime - lastTime >= 1.0) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }
        
        // ...渲染代码...
    }
}
```

### 监控粒子更新时间

```cpp
void Slime::updatePBF(float dt) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // ...PBF 计算...
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    if (duration.count() > 1000) {  // 超过 1ms
        std::cout << "⚠️ PBF 更新耗时: " << duration.count() << " μs" << std::endl;
    }
}
```

## 🎨 视觉调整

### 调整粒子大小

```cpp
// 更大的粒子更容易看到
slime->setParticleRadius(0.15f);  // 默认 0.08
```

### 调整史莱姆颜色

在 `setupDemoData()` 中：

```cpp
slimeShader->begin();
slimeShader->setVec3("uSlimeColor", glm::vec3(1.0f, 0.0f, 0.0f));  // 红色
slimeShader->end();
```

### 减少粒子数量（测试）

```cpp
// 少量粒子更容易调试
Slime* slime = new Slime(this, glm::vec3(-5.0f, 2.0f, 0.0f), 
                         1.5f, 50, slimeShader, 0);  // 50 而不是 500
```

## 💡 常见问题

### Q: 粒子在地板下面？

**A**: 检查初始 Y 坐标。史莱姆初始位置是 `(-5, 2, 0)`，地板在 `y = -5`，所以应该在空中。

### Q: 粒子太小看不见？

**A**: 增大 `m_particleRadius`：

```cpp
m_particleRadius = 0.2f;  // 在构造函数中
```

### Q: 深度测试问题？

**A**: 确保启用了深度测试：

```cpp
glEnable(GL_DEPTH_TEST);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

### Q: 粒子闪烁？

**A**: 可能是 Z-fighting。调整近远裁剪平面：

```cpp
camera->setNearFar(0.1f, 1000.0f);
```

### Q: 半透明混合问题？

**A**: 确保混合模式正确：

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// 禁用深度写入（可选，用于完全透明效果）
glDepthMask(GL_FALSE);
// 渲染后恢复
glDepthMask(GL_TRUE);
```

## 🚀 下一步

如果粒子仍然不可见，请：

1. 检查控制台输出的调试信息
2. 使用 RenderDoc 捕获一帧分析
3. 尝试简化shader测试
4. 检查相机视角和史莱姆位置

修复后记得移除调试输出以提高性能！
