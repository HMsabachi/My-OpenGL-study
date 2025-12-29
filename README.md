# 最新
##  学习OpenGL

---

## 📝 更新日志

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
│   └── object/          # 游戏对象
│       ├── object.h/cpp    # 对象基类
│       ├── cube.h/cpp      # 立方体
│       ├── sphere.h/cpp    # 球体
│       └── floor.h/cpp     # 地板
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
- ✅ 参数化几何体生成（立方体、球体）
- ✅ 对象化渲染系统
- ✅ 物理引擎集成（ReactPhysics3D）

### 待实现功能
- ⬜ 光照系统（Phong/PBR）
- ⬜ 阴影映射
- ⬜ 帧缓冲与后处理
- ⬜ 天空盒
- ⬜ 模型加载（Assimp）
- ⬜ 物理碰撞响应
- ⬜ 粒子系统

---

## 📖 参考资料
- [LearnOpenGL CN](https://learnopengl-cn.github.io/)
- [OpenGL 官方文档](https://www.opengl.org/documentation/)
- [ReactPhysics3D 文档](https://www.reactphysics3d.com/documentation.html)

---

## 📄 许可证
本项目仅用于学习目的。