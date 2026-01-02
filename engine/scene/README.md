# 🎮 新引擎框架架构文档

## 📁 文件结构

```
engine/scene/
├── Component.h           # 组件基类
├── GameObject.h          # 游戏对象
├── GameObject.cpp
├── Transform.h           # 变换组件
├── Transform.cpp
├── SceneManager.h        # 场景管理器
├── SceneManager.cpp
├── LightComponent.h      # 灯光组件系统
└── README.md            # 本文档
```

## 🏗️ 架构设计

### 核心概念

本框架采用**组件模式（Component Pattern）**设计，类似 Unity/Unreal 引擎：

1. **GameObject（游戏对象）**
   - 场景中的基本实体
   - 本身只包含名称、标签、激活状态
   - 所有功能通过添加组件实现

2. **Component（组件）**
   - 独立的功能模块
   - 可以添加到任何 GameObject 上
   - 提供生命周期回调：`awake()`, `start()`, `update()`, `lateUpdate()`, `onDestroy()`

3. **Transform（变换组件）**
   - 每个 GameObject 必有的组件
   - 管理位置、旋转、缩放
   - 支持父子层级关系

4. **Scene（场景）**
   - 管理 GameObject 的容器
   - 负责对象的生命周期管理
   - 提供对象查找功能

5. **SceneManager（场景管理器）**
   - 管理多个场景
   - 支持场景切换
   - 统一调度更新

## 📝 使用示例

### 1. 基本使用

```cpp
#include "scene/SceneManager.h"
#include "scene/LightComponent.h"

// 在 Engine 中创建场景管理器
SceneManager* sceneManager = new SceneManager(engine);

// 创建场景
Scene* mainScene = sceneManager->createScene("MainScene");

// 创建游戏对象
GameObject* player = mainScene->createGameObject("Player");

// 设置位置
player->getTransform()->setLocalPosition(glm::vec3(0, 0, 0));

// 添加组件（示例：自定义组件）
// auto* playerController = player->addComponent<PlayerController>();
```

### 2. 灯光系统使用

```cpp
// 创建方向光（太阳光）
GameObject* sunObject = mainScene->createGameObject("Sun");
auto* sunLight = sunObject->addComponent<DirectionalLight>(
    glm::vec3(0, -1, -0.5f),     // 方向
    glm::vec3(1.0f, 0.95f, 0.8f), // 暖色调
    1.0f                          // 强度
);

// 创建点光源（灯泡）
GameObject* lampObject = mainScene->createGameObject("Lamp");
lampObject->getTransform()->setLocalPosition(glm::vec3(5, 2, 0));
auto* lampLight = lampObject->addComponent<PointLight>(
    glm::vec3(1.0f, 0.8f, 0.6f),  // 橙色
    2.0f,                          // 强度
    10.0f                          // 范围
);

// 创建聚光灯（手电筒）
GameObject* flashlightObject = mainScene->createGameObject("Flashlight");
auto* flashlight = flashlightObject->addComponent<SpotLight>(
    glm::vec3(0, 0, -1),           // 方向
    glm::vec3(1.0f),               // 白色
    3.0f,                          // 强度
    15.0f,                         // 内锥角
    25.0f,                         // 外锥角
    20.0f                          // 范围
);
```

### 3. 层级关系

```cpp
// 创建父对象
GameObject* parent = mainScene->createGameObject("Parent");
parent->getTransform()->setLocalPosition(glm::vec3(5, 0, 0));

// 创建子对象
GameObject* child = mainScene->createGameObject("Child");
child->setParent(parent);
child->getTransform()->setLocalPosition(glm::vec3(1, 0, 0)); // 相对父对象的位置

// 子对象的世界坐标 = (6, 0, 0)
glm::vec3 worldPos = child->getTransform()->getWorldPosition();
```

### 4. 组件查找

```cpp
// 按名称查找对象
GameObject* player = mainScene->findObjectByName("Player");

// 按标签查找对象
player->setTag("Player");
GameObject* playerByTag = mainScene->findObjectByTag("Player");

// 查找所有带特定组件的对象
auto objectsWithLights = mainScene->findObjectsWithComponent<LightComponent>();
```

## 🔌 自定义组件示例

```cpp
// PlayerController.h
class PlayerController : public Component {
private:
    float moveSpeed;
    
public:
    PlayerController(GameObject* owner, float speed = 5.0f)
        : Component(owner, "PlayerController"),
          moveSpeed(speed) {}
    
    void start() override {
        // 初始化（第一帧前调用）
        std::cout << "Player started!" << std::endl;
    }
    
    void update(float deltaTime) override {
        // 每帧更新
        auto* transform = m_owner->getTransform();
        
        // 简单移动
        if (/* 按下W键 */) {
            glm::vec3 forward = transform->forward();
            transform->translate(forward * moveSpeed * deltaTime);
        }
    }
    
    const char* getTypeName() const override { return "PlayerController"; }
};

// 使用
GameObject* player = mainScene->createGameObject("Player");
auto* controller = player->addComponent<PlayerController>(10.0f);
```

## 🎯 与旧代码的集成

### 方案 1：逐步迁移（推荐）

```cpp
// 在 Engine 中添加新的场景管理器
class Engine {
public:
    SceneManager* newSceneManager;  // 新框架
    Scene* oldScene;                // 旧框架（保留兼容）
    
    void init() {
        // 初始化新框架
        newSceneManager = new SceneManager(this);
        
        // 保留旧代码
        oldScene = new Scene(this);
    }
};
```

### 方案 2：适配器模式

```cpp
// ObjectAdapter.h - 将旧的 Object 包装成 Component
class ObjectAdapter : public Component {
private:
    Object* m_legacyObject;  // 旧的 Object 指针
    
public:
    ObjectAdapter(GameObject* owner, Object* legacyObject)
        : Component(owner, "ObjectAdapter"),
          m_legacyObject(legacyObject) {}
    
    void update(float deltaTime) override {
        if (m_legacyObject) {
            m_legacyObject->update(deltaTime);
        }
    }
    
    void render() {
        if (m_legacyObject) {
            m_legacyObject->render();
        }
    }
};

// 使用
GameObject* cubeObject = scene->createGameObject("Cube");
Cube* legacyCube = new Cube(...);
cubeObject->addComponent<ObjectAdapter>(legacyCube);
```

## 📊 性能优化建议

### 1. 对象池
```cpp
// 避免频繁创建/销毁对象
class ObjectPool {
    std::vector<std::unique_ptr<GameObject>> pool;
public:
    GameObject* acquire() { /* 从池中获取 */ }
    void release(GameObject* obj) { /* 返回池中 */ }
};
```

### 2. 空间查询优化
```cpp
// 使用八叉树加速对象查找
class Octree {
    // 将场景空间分成 8 个子空间
    // 快速查找指定区域内的对象
};
```

### 3. 组件缓存
```cpp
// 缓存常用组件指针
class MyScript : public Component {
private:
    Transform* cachedTransform;  // 缓存 Transform
    
public:
    void awake() override {
        cachedTransform = m_owner->getTransform();  // 只获取一次
    }
    
    void update(float deltaTime) override {
        cachedTransform->translate(...);  // 直接使用缓存
    }
};
```

## 🔮 未来扩展

已经规划但尚未实现的功能：

1. **渲染组件**
   - `MeshRenderer` - 网格渲染器
   - `SpriteRenderer` - 精灵渲染器
   - `Camera` - 相机组件

2. **物理组件**
   - `Rigidbody` - 刚体组件
   - `Collider` - 碰撞体组件
   - `PhysicsMaterial` - 物理材质

3. **动画系统**
   - `Animator` - 动画控制器
   - `AnimationClip` - 动画片段

4. **粒子系统**
   - `ParticleSystem` - 粒子发射器

5. **音频系统**
   - `AudioSource` - 音频源组件
   - `AudioListener` - 音频监听器

6. **UI系统**
   - `Canvas` - 画布
   - `UIElement` - UI元素基类

## 📚 参考资料

- Unity Component System: https://docs.unity3d.com/Manual/Components.html
- Unreal Engine Actor Component: https://docs.unrealengine.com/en-US/ProgrammingAndScripting/ProgrammingWithCPP/UnrealArchitecture/Actors/Components/
- Game Programming Patterns: https://gameprogrammingpatterns.com/component.html

## 🐛 已知问题

1. **性能问题**：大量对象时 `dynamic_cast` 可能成为瓶颈
   - 解决方案：使用类型ID代替 `dynamic_cast`

2. **内存管理**：组件生命周期管理需要更细粒度控制
   - 解决方案：引入智能指针和对象池

3. **序列化**：暂不支持场景保存/加载
   - 解决方案：实现 JSON/XML 序列化

## ✅ 快速开始检查清单

- [ ] 将新框架文件添加到 CMakeLists.txt
- [ ] 在 Engine 中创建 SceneManager
- [ ] 创建第一个测试场景
- [ ] 创建第一个 GameObject 并添加组件
- [ ] 测试对象生命周期（awake, start, update）
- [ ] 测试层级关系（父子对象）
- [ ] 集成灯光系统到渲染管线

---

**版本**: 1.0
**最后更新**: 2024
**作者**: AI Assistant
