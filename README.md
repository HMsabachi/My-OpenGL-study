# 最新
##  学习OpenGL

---

## 📝 更新日志

### v0.5.0 - PBF 流体模拟系统 (2025-01-XX)
**重大更新：基于物理的流体模拟（Position Based Fluids）**

#### 🎯 主要变更

- **Slime 史莱姆类**（全新）
  - ✨ **PBF 算法实现**：Position Based Fluids 粒子流体模拟
  - ✨ **1000 粒子系统**：实时模拟 1000 个流体粒子，保持流动性
  - ✨ **密度约束**：通过 SPH 核函数计算密度，自动保持流体体积
  - ✨ **向心力系统**：可调节的向心力，控制史莱姆聚合/分散状态
  - ✨ **粘性模拟**：XSPH 粘性模型，让流体运动更平滑
  - ✨ **表面张力**：相邻粒子吸引力，形成光滑的流体表面
  - 🎨 **水滴形状控制**：上方粒子受更强向心力，形成自然的水滴形态
  - 🎮 **玩家可控制**：通过施加力来移动整个史莱姆

- **物理碰撞检测优化**
  - ✨ **Raycast 预测性碰撞**：沿速度方向发射射线，提前检测障碍物
  - ✨ **只检测移动粒子**：静止粒子跳过碰撞检测，提升性能
  - ✨ **弹性与摩擦**：可配置的弹性系数和摩擦系数
  - ✨ **位置修正**：精确的穿透修正，防止粒子穿透障碍物
  - ✨ **保持流动性**：不创建刚体，粒子完全由 PBF 控制
  - 🔧 **可被场景切割**：史莱姆可以流过障碍物，粒子可分离

- **PlayerController 增强**
  - ✨ **物体控制模式**：按 `C` 键切换摄像机/物体控制模式
  - ✨ **相对方向移动**：移动方向相对于摄像机，更直观的控制
  - ✨ **力控制系统**：通过施加力而非直接设置速度，物理更真实
  - ✨ **可调节参数**：移动速度和施加力独立配置

- **PBF 算法核心**
  - 🔬 **SPH 核函数**：
    - Poly6 Kernel：用于密度计算
    - Spiky Gradient：用于压力梯度计算
  - 🔬 **约束求解器**：
    - 拉格朗日乘数法求解密度约束
    - 迭代位置修正（默认 3 次迭代）
    - 基于约束的速度更新
  - 🔬 **空间加速**：
    - 哈希表空间分区
    - 邻居搜索半径：粒子半径 × 4
    - 只检查相邻 27 个格子

#### 💡 技术细节

**PBF 模拟流程**：
```
1. 施加外力（重力 + 控制力）
2. 预测位置（速度积分）
3. 构建空间哈希表
4. 查找每个粒子的邻居
5. 迭代求解约束：
   - 计算密度
   - 计算 lambda（拉格朗日乘数）
   - 计算位置修正
   - 应用位置修正
6. 更新速度
7. 应用向心力（保持形状）
8. 应用粘性（平滑运动）
9. 物理碰撞检测（Raycast）
10. 更新渲染数据
```

**关键参数**：
| 参数 | 值 | 说明 |
|------|-----|------|
| `particleCount` | 1000 | 粒子数量 |
| `particleRadius` | 0.12 | 单个粒子半径 |
| `restDensity` | 70.0 | 静止密度（SPH） |
| `epsilon` | 600.0 | 松弛参数（防止除零） |
| `solverIterations` | 3 | 约束求解迭代次数 |
| `cohesionStrength` | 50.0 | 向心力强度 |
| `viscosity` | 0.05 | 粘性系数 |
| `restitution` | 0.3 | 碰撞弹性 |
| `friction` | 0.4 | 碰撞摩擦 |

#### 🎮 控制说明

**模式切换**：
- **C 键**：切换摄像机/史莱姆控制模式
- **R 键**：切换向心力（50.0 ↔ 0.0）
  - 向心力 = 50：史莱姆保持聚合
  - 向心力 = 0：史莱姆自由扩散

**史莱姆控制**（切换到物体模式后）：
- **W/A/S/D**：水平移动
- **Space**：向上移动
- **Left Shift**：向下移动
- **鼠标**：旋转视角（不移动摄像机）

#### 🏗️ 架构设计

**为什么不用刚体？**
- ❌ **传统方案**：为每个粒子创建刚体
  - 失去流动性（刚体硬约束）
  - 性能开销巨大（1000 个刚体碰撞）
  - PBF 与物理引擎约束冲突

- ✅ **当前方案**：PBF + 物理查询
  - 保持完美流动性（软约束）
  - 高效碰撞检测（Raycast）
  - 可被场景切割（粒子独立）
  - 性能优秀（0 个刚体）

**碰撞检测策略**：
```cpp
// 预测性碰撞检测
glm::vec3 rayDir = normalize(velocity);
float rayLength = speed * dt + checkDistance;

// 只检测移动粒子
if (speed < 0.01f) continue;

// Raycast 检测障碍物
Ray ray(position, position + rayDir * rayLength);
world->raycast(ray, callback);

// 位置修正 + 速度反弹
if (hasHit) {
    position += hitNormal * penetration;
    velocity -= (1 + restitution) * (velocity · normal) * normal;
}
```

#### 🎨 渲染优化

- **实例化渲染**：1000 个粒子用 1 次 Draw Call
- **GPU 矩阵计算**：变换矩阵在 CPU 组装，批量上传
- **动态缓冲**：每帧更新实例化缓冲（`glBufferSubData`）
- **球体网格复用**：所有粒子共享同一个球体网格

#### 📊 性能分析

**测试环境**：
- 粒子数：1000
- 帧率：60 FPS
- 碰撞物体：11 个（1 地板 + 10 立方体）

**性能开销**：
- PBF 模拟：~3ms
- 空间哈希：~0.5ms
- 邻居搜索：~1ms
- 约束求解：~1.5ms
- 碰撞检测：~1ms（Raycast × 移动粒子数）
- 渲染更新：~0.5ms

**优化技巧**：
1. 只检测移动粒子（跳过静止粒子）
2. 空间哈希加速邻居搜索（O(n) → O(1)）
3. 限制邻居数量（最多检查 27 个格子）
4. 实例化渲染（1 Draw Call）

#### 📂 涉及文件
- `engine/object/slime.h` & `.cpp` - 史莱姆 PBF 流体类
- `engine/playerController.h` & `.cpp` - 玩家控制器增强
- `assets/shaders/slime_vertex.glsl` - 史莱姆顶点着色器
- `assets/shaders/slime_fragment.glsl` - 史莱姆片段着色器
- `engine/engine.cpp` - 史莱姆创建与参数配置

#### 🐛 Bug 修复
- 修复 Raycast 过度反弹问题（调整弹性系数）
- 修正粒子穿透地面问题（增加检测距离）
- 优化向心力计算，避免粒子过度聚合
- 修复控制力分配不均导致的漂移

#### 🔬 学习资源
- [Position Based Fluids (Macklin & Müller, 2013)](https://mmacklin.com/pbf_sig_preprint.pdf)
- [Smoothed Particle Hydrodynamics (SPH)](https://en.wikipedia.org/wiki/Smoothed-particle_hydrodynamics)
- [PBD/XPBD 教程](https://matthias-research.github.io/pages/publications/posBasedDyn.pdf)

---

### v0.4.0 - 物理引擎集成与场景管理系统 (2025-01-XX)
**重大更新：完整的物理模拟和对象管理架构**

#### 🎯 主要变更

- **Object 基类重构**
  - ✨ 添加完整的变换系统：旋转（四元数）、缩放、位置
  - ✨ 物理类型枚举：`PhysicsType` (NONE, STATIC, KINEMATIC, DYNAMIC)
  - ✨ 碰撞形状枚举：`CollisionShape` (NONE, BOX, SPHERE, PLANE, CAPSULE)
  - ✨ 物理初始化：`initPhysics()` 方法创建刚体和碰撞体
  - ✨ 物理同步：`syncFromPhysics()` 和 `syncToPhysics()` 自动同步物理状态
  - ✨ 完整的变换API：支持 getter/setter 模式访问所有变换属性
  - 🔧 基类现在不允许直接实例化，子类必须实现 `render()` 和 `collideWith()`

- **Scene 场景管理系统**（全新）
  - ✨ 统一的对象生命周期管理：使用 `std::unique_ptr` 自动管理内存
  - ✨ 集中式更新和渲染：`update()` 和 `render()` 方法
  - ✨ 物理世界集成：在每帧自动更新 ReactPhysics3D 物理引擎
  - ✨ 对象清理机制：`cleanupInactiveObjects()` 自动清理非活跃对象
  - ✨ 类型查找功能：模板方法 `findObjectsByType<T>()` 按类型查找对象
  - 📊 统计功能：获取总对象数和活跃对象数

- **Plane 地板类**（全新）
  - ✨ 参数化平面网格生成：可自定义 XZ 平面尺寸
  - ✨ 纹理平铺支持：`setTextureRepeat()` 方法控制纹理重复
  - ✨ 物理地板：作为静态物理体，支持碰撞检测
  - 🎨 使用 EBO 优化渲染（4个顶点，6个索引）

- **物理引擎完整集成**
  - ✨ ReactPhysics3D 世界管理：在 Engine 中统一创建和配置
  - ✨ 重力设置：默认 -9.81 m/s² (Y轴向下)
  - ✨ 刚体类型支持：静态、运动学、动态
  - ✨ 多种碰撞形状：盒子、球体、平面、胶囊体
  - ✨ 质量和密度配置：动态物体支持自定义质量
  - 🔄 自动物理同步：动态物体每帧从物理引擎同步变换

- **现有类适配**
  - 🔧 **Cube 类**：
    - 使用四元数旋转替代欧拉角
    - 支持物理模拟（可选）
    - 渲染时使用父类变换信息
  - 🔧 **Sphere 类**：
    - 添加缩放支持
    - 支持物理模拟（可选）
    - 使用父类变换系统
  - 🔧 **Engine 类**：
    - 集成 Scene 管理器
    - 移除直接管理的 `cubes` 和 `spheres` 向量
    - 物理世界初始化和配置
    - 通过 Scene 统一管理所有对象

#### 🎮 示例场景
在 `Engine::setupDemoData()` 中创建了演示场景：
- 🏗️ **地板**：50x50 单位的静态物理平面，10x10 纹理平铺
- 🎲 **动态立方体**：前5个立方体具有物理模拟，会受重力影响下落
- ⚫ **动态球体**：1个球体，具有完整的物理碰撞
- 🎨 **静态立方体**：其余5个立方体仅作为视觉装饰

#### 💡 使用示例

```cpp
// 1. 创建物理地板
Plane* floor = new Plane(engine, glm::vec3(0.0f, -5.0f, 0.0f), 
                         glm::vec2(50.0f, 50.0f), shader, texture);
floor->setTextureRepeat(10.0f, 10.0f);
floor->initPhysics(Object::PhysicsType::STATIC, 
                   Object::CollisionShape::PLANE, 
                   glm::vec3(50.0f, 0.2f, 50.0f));
scene->addObject(floor);

// 2. 创建动态物理立方体
Cube* cube = new Cube(engine, position, glm::vec3(1.0f), 
                      shader, tex1, tex2);
cube->initPhysics(Object::PhysicsType::DYNAMIC,  // 动态物理体
                  Object::CollisionShape::BOX,    // 盒子碰撞形状
                  glm::vec3(1.0f),                // 形状尺寸
                  1.0f);                          // 质量(kg)
scene->addObject(cube);

// 3. 场景自动管理生命周期
scene->update(deltaTime);  // 更新物理和所有对象
scene->render();           // 渲染所有活跃对象
scene->cleanupInactiveObjects();  // 清理销毁的对象
```

#### 🏗️ 架构优势
1. **统一管理**：所有游戏对象通过 Scene 集中管理，代码更清晰
2. **自动内存管理**：使用智能指针，避免内存泄漏
3. **物理引擎深度集成**：完整支持 ReactPhysics3D 的所有特性
4. **灵活扩展**：Object 基类设计良好，易于派生新对象类型
5. **生命周期控制**：支持对象激活/销毁，自动清理机制

#### 🐛 Bug 修复
- 修复 Cube 旋转设置时的递归调用问题（使用 `Object::setRotation`）
- 修正 Plane 类中 VAO 的 EBO 设置（使用 `addEBO` 而不是 `setEBO`）
- 优化物理同步逻辑，避免静态物体不必要的变换更新

#### 📂 涉及文件
- `engine/object/object.h` & `.cpp` - Object 基类重构
- `engine/scene.h` & `.cpp` - 新增场景管理系统
- `engine/object/plane.h` & `.cpp` - 新增地板类
- `engine/object/cube.cpp` - 适配物理系统
- `engine/object/sphere.cpp` - 适配物理系统
- `engine/engine.h` & `.cpp` - 集成 Scene 和物理引擎
- `engine/CMakeLists.txt` - 添加新文件编译配置

---

### v0.3.0 - Shader Uniform 系统重构 (2025-01-XX)
**核心改进：统一的 Shader 管理系统**

#### 🎯 主要变更
- **Shader 类优化**
  - 添加 `m_isBound` 状态跟踪，避免频繁的 Shader 切换
  - 改进 `set*` 方法：批量设置 Uniform 时不再重复调用 `begin()`/`end()`
  - 性能提升：减少不必要的 GPU 状态切换

- **Engine 全局 Uniform 管理**
  - 新增 `updateGlobalUniforms()` 方法，统一管理所有 Shader 的 View/Projection 矩阵
  - 清晰的职责划分：全局 Uniform（Engine）vs 局部 Uniform（Object）
  - 统一的更新时机：初始化、每帧、窗口大小改变

- **对象渲染规范化**
  - 标准化 `Cube` 和 `Sphere` 的渲染流程
  - 移除冗余的 `shader->begin()` 调用
  - 纹理单元配置只在初始化时执行一次

#### 📂 涉及文件
- `glFrameWork/shader.h` & `.cpp` - Shader 核心类优化
- `engine/engine.h` & `.cpp` - 全局 Uniform 管理
- `engine/object/cube.cpp` - 立方体渲染优化
- `engine/object/sphere.cpp` - 球体渲染优化

---

### v0.2.0 - 球体几何体生成器 (2025-01-XX)
**新增功能：参数化球体网格生成**

#### 🎯 主要变更
- **Widgets 模块扩展**
  - 实现 `createSphere()` 函数，支持自定义半径、经纬度细分
  - 顶点数据格式：Position (3f) + Normal (3f) + TexCoord (2f)
  - 自动生成球坐标系到笛卡尔坐标系的转换
  - 正确的 CCW 索引顺序，兼容 OpenGL 默认背面剔除

- **Sphere 对象类**
  - 继承自 `Object` 基类
  - 集成 `widgets::createSphere()` 生成网格
  - 支持纹理映射和 Shader 配置
  - 修复 VBO/EBO 生命周期管理问题（使用 `shared_ptr`）

- **配套 Shader**
  - `sphere_vertex.glsl` - 顶点着色器，支持法线变换
  - `sphere_fragment.glsl` - 片段着色器，纹理映射

#### 🐛 Bug 修复
- 修复顶点数据重复问题（移除冗余的位置数据写入）
- 修复 Buffer 对象过早销毁导致的渲染错误
- 修正 CMake 文件扫描模式（`GLOB_RECURSE` 语法）

#### 📂 涉及文件
- `wrapper/widgets.h` & `.cpp` - 球体生成器
- `engine/object/sphere.h` & `.cpp` - 球体对象类
- `assets/shaders/sphere_vertex.glsl` - 球体顶点着色器
- `assets/shaders/sphere_fragment.glsl` - 球体片段着色器
- `engine/CMakeLists.txt` - 构建配置修正
- `wrapper/CMakeLists.txt` - 构建配置修正

---

### v0.1.0 - 项目基础架构 (2025-01-XX)
**初始化：OpenGL 学习框架搭建**

#### 🎯 主要特性
- **模块化架构**
  - `glFrameWork/` - OpenGL 封装层（Shader, Buffer, VAO, Texture）
  - `engine/` - 游戏引擎核心（Engine, Camera, Object 系统）
  - `wrapper/` - 辅助工具（错误检查、几何体生成）
  - `application/` - 窗口管理（GLFW 封装）

- **核心功能**
  - Shader 系统：自动编译、链接、Uniform 设置
  - Buffer 系统：模板化 VBO/EBO，支持多种数据类型
  - VAO 系统：字符串化布局配置（如 `"3f 2f"`）
  - 纹理管理：TextureManager 单例模式
  - 相机系统：FPS 风格，支持键盘+鼠标控制

- **物理引擎集成**
  - ReactPhysics3D 集成
  - Object 基类支持刚体绑定

- **示例对象**
  - `Cube` 类：带纹理的立方体
  - 支持位置、旋转、缩放变换

#### 📂 涉及文件
- `glFrameWork/*` - OpenGL 封装
- `engine/*` - 引擎核心
- `application/*` - 窗口系统
- `wrapper/*` - 工具库
- `CMakeLists.txt` - 根构建配置

---

## 🏗️ 项目结构

```
最新/
├── glFrameWork/         # OpenGL 封装层
│   ├── shader.h/cpp     # Shader 管理
│   ├── buffers.h        # VBO/EBO/VAO 封装
│   ├── texture.h/cpp    # 纹理系统
│   └── ...
├── engine/              # 游戏引擎核心
│   ├── engine.h/cpp     # 引擎主类
│   ├── camera.h/cpp     # 相机系统
│   ├── scene.h/cpp      # 场景管理系统
│   ├── playerController.h/cpp  # 玩家控制器 ✨
│   └── object/          # 游戏对象
│       ├── object.h/cpp    # 对象基类（物理引擎集成）
│       ├── cube.h/cpp      # 立方体
│       ├── sphere.h/cpp    # 球体
│       ├── plane.h/cpp     # 地板
│       └── slime.h/cpp     # 史莱姆 PBF 流体 ✨
├── wrapper/             # 辅助工具
│   ├── widgets.h/cpp    # 几何体生成器
│   └── checkError.h     # OpenGL 错误检查
├── application/         # 窗口系统
│   └── application.h/cpp
├── assets/              # 资源文件
│   ├── shaders/         # GLSL 着色器
│   │   ├── slime_vertex.glsl    # 史莱姆顶点着色器 ✨
│   │   └── slime_fragment.glsl  # 史莱姆片段着色器 ✨
│   └── textures/        # 纹理图片
└── CMakeLists.txt       # 根构建配置
```

---

## 🚀 构建与运行

### 环境要求
- **CMake**: >= 3.12
- **编译器**: MSVC 2022 / GCC / Clang (支持 C++17)
- **依赖库**: 
  - GLFW3
  - GLM
  - ReactPhysics3D
  - stb_image

### 构建步骤
```bash
# 1. 克隆仓库
git clone https://github.com/HMsabachi/My-OpenGL-study.git
cd 最新

# 2. 配置 CMake
cmake -B build -G Ninja

# 3. 编译
cmake --build build --config Release

# 4. 运行
./build/glStudy
```

---

## 📚 学习笔记

### 已实现功能
- ✅ 基础 OpenGL 渲染管线
- ✅ Shader 编译与 Uniform 管理
- ✅ VAO/VBO/EBO 封装
- ✅ 纹理加载与绑定
- ✅ FPS 相机系统
- ✅ 参数化几何体生成（立方体、球体、平面）
- ✅ 对象化渲染系统
- ✅ 物理引擎集成（ReactPhysics3D）
- ✅ 场景管理系统
- ✅ 刚体物理模拟（重力、碰撞）
- ✅ 四元数旋转系统
- ✅ **PBF 流体模拟**（史莱姆）⭐
- ✅ **玩家控制系统**（摄像机/物体切换）⭐
- ✅ **粒子系统**（1000 粒子实时模拟）⭐
- ✅ **实例化渲染**（GPU 加速）⭐

### 待实现功能
- ⬜ 光照系统（Phong/PBR）
- ⬜ 阴影映射
- ⬜ 帧缓冲与后处理
- ⬜ 天空盒
- ⬜ 模型加载（Assimp）
- ⬜ 更多流体特效（泡沫、水花）
- ⬜ 流体与刚体双向交互
- ⬜ GPU 粒子模拟（Compute Shader）
- ⬜ 音频系统
- ⬜ GUI 系统（ImGui）

---

## 🎮 控制说明

### 摄像机控制（默认模式）
- **W/S/A/D**: 前后左右移动
- **Space**: 向上移动
- **Left Shift**: 向下移动
- **鼠标移动**: 视角旋转
- **Left Alt**: 切换鼠标捕获模式

### 史莱姆控制（按 C 切换）
- **W/S/A/D**: 水平移动史莱姆
- **Space**: 向上移动
- **Left Shift**: 向下移动
- **鼠标移动**: 旋转视角（摄像机位置不变）
- **C**: 切换回摄像机模式

### 调试功能
- **R**: 切换史莱姆向心力（聚合 ↔ 扩散）
- **Z**: 重置摄像机位置

---

## 📖 参考资料
- [LearnOpenGL CN](https://learnopengl-cn.github.io/)
- [OpenGL 官方文档](https://www.opengl.org/documentation/)
- [ReactPhysics3D 文档](https://www.reactphysics3d.com/documentation.html)
- [GLM 文档](https://glm.g-truc.net/0.9.9/index.html)
- [Position Based Fluids 论文](https://mmacklin.com/pbf_sig_preprint.pdf) ⭐
- [SPH 粒子流体教程](https://lucasschuermann.com/writing/implementing-sph-in-2d) ⭐

---

## 🎥 效果展示

### 史莱姆流体模拟
- 1000 个粒子实时模拟
- 自然的流动效果
- 可以流过障碍物，被场景"切割"
- 碰撞反弹 + 表面张力
- 玩家可控制移动

### 物理交互
- 史莱姆与立方体碰撞
- 史莱姆在地面上滑动
- 动态立方体受重力下落
- 完整的刚体物理模拟

---

## 📄 许可证
本项目仅用于学习目的。