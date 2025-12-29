# 最新
##  学习OpenGL

---

## 📝 更新日志

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
│   ├── scene.h/cpp      # 场景管理系统 ✨
│   └── object/          # 游戏对象
│       ├── object.h/cpp    # 对象基类（物理引擎集成）
│       ├── cube.h/cpp      # 立方体
│       ├── sphere.h/cpp    # 球体
│       └── plane.h/cpp     # 地板 ✨
├── wrapper/             # 辅助工具
│   ├── widgets.h/cpp    # 几何体生成器
│   └── checkError.h     # OpenGL 错误检查
├── application/         # 窗口系统
│   └── application.h/cpp
├── assets/              # 资源文件
│   ├── shaders/         # GLSL 着色器
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

### 待实现功能
- ⬜ 光照系统（Phong/PBR）
- ⬜ 阴影映射
- ⬜ 帧缓冲与后处理
- ⬜ 天空盒
- ⬜ 模型加载（Assimp）
- ⬜ 物理碰撞响应回调
- ⬜ 粒子系统
- ⬜ 音频系统
- ⬜ GUI 系统（ImGui）

---

## 🎮 控制说明

### 相机控制
- **W/S/A/D**: 前后左右移动
- **Space**: 向上移动
- **Left Shift**: 向下移动
- **鼠标移动**: 视角旋转
- **Left Alt**: 切换鼠标捕获模式

---

## 📖 参考资料
- [LearnOpenGL CN](https://learnopengl-cn.github.io/)
- [OpenGL 官方文档](https://www.opengl.org/documentation/)
- [ReactPhysics3D 文档](https://www.reactphysics3d.com/documentation.html)
- [GLM 文档](https://glm.g-truc.net/0.9.9/index.html)

---

## 📄 许可证
本项目仅用于学习目的。