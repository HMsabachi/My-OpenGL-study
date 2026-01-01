# 史莱姆动态网格渲染系统

## 功能概述

史莱姆现在支持两种渲染模式：
1. **粒子球体模式**：显示每个粒子为一个小球体
2. **动态网格模式**：使用 Marching Cubes 算法生成光滑的动态网格表面

## 使用方法

### 按键控制

- **M 键**：切换渲染模式（粒子/网格）
- **C 键**：切换控制模式（摄像机/物体）
- **R 键**：开关向心力（史莱姆凝聚力）
- **WASD**：控制史莱姆移动（在物体控制模式下）
- **Space/Shift**：上升/下降
- **Left Alt**：切换鼠标捕获

### 渲染模式特性

#### 粒子模式
- 实时显示所有粒子
- 性能较好
- 可以看到粒子的运动细节
- 使用 `slime_vertex.glsl` 和 `slime_fragment.glsl`

#### 网格模式
- 生成光滑的表面
- 使用密度场和 Marching Cubes 算法
- 支持半透明和高级光照效果（菲涅尔、次表面散射）
- 使用 `slime_mesh_vertex.glsl` 和 `slime_mesh_fragment.glsl`
- 默认每秒更新 20 次网格（0.05秒间隔）

## 技术实现

### 架构

```
Slime (slime.h/cpp)
├── 粒子模拟 (PBF算法)
├── 渲染系统
│   ├── 粒子模式 (实例化渲染)
│   └── 网格模式 (动态网格生成)
│       ├── DensityField (密度场)
│       ├── MarchingCubes (网格生成)
│       └── 着色器系统
└── 控制系统
    ├── PlayerController
    └── SlimeController
```

### 核心组件

#### 1. DensityField (densityField.h/cpp)
- 将粒子光栅化到3D网格
- 使用高斯模糊平滑密度场
- 支持动态边界更新

#### 2. MarchingCubes (marchingCubes.h/cpp)
- 实现经典 Marching Cubes 算法
- 从密度场提取等值面
- 自动计算顶点法线（使用密度梯度）

#### 3. 着色器系统
- **slime_mesh_vertex.glsl**：顶点变换
- **slime_mesh_fragment.glsl**：高级光照
  - 环境光 + 漫反射 + 镜面反射
  - 菲涅尔边缘高光
  - 次表面散射近似
  - 半透明效果

### 可调参数

在 Slime 类中可以调整以下参数：

```cpp
// 网格生成参数
slime->setMeshResolution(32);      // 密度场分辨率 (默认: 32)
slime->setIsoLevel(0.5f);           // 等值面阈值 (默认: 0.5)
slime->setBlurIterations(2);        // 模糊迭代次数 (默认: 2)

// 粒子参数
slime->setParticleRadius(0.12f);    // 粒子半径
slime->setRestDensity(70.0f);       // 流体密度
slime->setCohesionStrength(50.0f);  // 向心力强度
```

## 性能优化

- 使用 C++17 并行算法 (`std::execution::par_unseq`)
- 粒子光栅化并行化
- 空间哈希加速邻居查找
- 网格更新限流（每秒20次）
- 动态密度场边界（跟随史莱姆移动）

## 未来改进

- [ ] 完整的 Marching Cubes 查找表（当前为简化版）
- [ ] GPU 加速的密度场计算
- [ ] 更高级的平滑算法（例如：表面网格平滑）
- [ ] 纹理映射支持
- [ ] 多层次细节 (LOD)
- [ ] 自适应网格分辨率

## 已知问题

- Marching Cubes 查找表仅实现了前64项（完整实现需要256项）
- 高分辨率密度场可能影响性能
- 网格更新频率固定（未来可以改为自适应）

## 示例截图

（运行程序后按 M 键切换模式观看效果）
