// GameObject.h - 游戏对象类
#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include "Component.h"
#include "Transform.h"
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <typeinfo>

class Scene;

/**
 * @class GameObject
 * @brief 游戏对象 - 场景中的基本实体
 * 
 * GameObject 采用组件模式设计：
 * - 本身只包含基本信息（名称、激活状态、层级关系）
 * - 所有功能通过添加组件实现（渲染、物理、脚本等）
 * - 每个 GameObject 都有一个 Transform 组件（位置、旋转、缩放）
 */
class GameObject {
private:
    std::string m_name;                                  // 对象名称
    std::string m_tag;                                   // 标签（用于分组查找）
    bool m_isActive;                                     // 是否激活
    Scene* m_scene;                                      // 所属场景
    
    // 层级关系
    GameObject* m_parent;                                // 父对象
    std::vector<std::unique_ptr<GameObject>> m_children; // 子对象列表
    
    // 组件系统
    std::unique_ptr<Transform> m_transform;              // Transform 组件（必有）
    std::vector<std::unique_ptr<Component>> m_components; // 其他组件列表

    // 生命周期标记
    bool m_started;                                      // 是否已调用 start()
    bool m_markedForDestroy;                             // 是否标记为待销毁

public:
    /**
     * @brief 构造函数
     * @param name 对象名称
     * @param scene 所属场景
     */
    GameObject(const std::string& name = "GameObject", Scene* scene = nullptr);
    
    ~GameObject();

    // ===== 生命周期管理 =====
    
    /**
     * @brief 初始化（调用所有组件的 awake）
     */
    void awake();
    
    /**
     * @brief 开始（调用所有组件的 start）
     */
    void start();
    
    /**
     * @brief 更新（调用所有组件的 update）
     * @param deltaTime 时间增量
     */
    void update(float deltaTime);
    
    /**
     * @brief 固定更新（调用所有组件的 fixedUpdate）
     * @param fixedDeltaTime 固定时间增量
     */
    void fixedUpdate(float fixedDeltaTime);
    
    /**
     * @brief 延迟更新（调用所有组件的 lateUpdate）
     * @param deltaTime 时间增量
     */
    void lateUpdate(float deltaTime);
    
    /**
     * @brief 标记为待销毁（将在帧结束时销毁）
     */
    void destroy();
    
    /**
     * @brief 检查是否标记为待销毁
     */
    bool isMarkedForDestroy() const { return m_markedForDestroy; }

    // ===== 组件管理 =====
    
    /**
     * @brief 添加组件
     * @tparam T 组件类型
     * @tparam Args 构造参数类型
     * @param args 组件构造参数
     * @return 指向新组件的指针
     */
    template<typename T, typename... Args>
    T* addComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        
        auto component = std::make_unique<T>(this, std::forward<Args>(args)...);
        T* ptr = component.get();
        
        m_components.push_back(std::move(component));
        
        // 如果对象已经启动，立即调用组件的 awake
        if (m_started) {
            ptr->awake();
        }
        
        return ptr;
    }
    
    /**
     * @brief 获取组件
     * @tparam T 组件类型
     * @return 指向组件的指针，如果不存在返回 nullptr
     */
    template<typename T>
    T* getComponent() const {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        
        for (const auto& component : m_components) {
            if (T* ptr = dynamic_cast<T*>(component.get())) {
                return ptr;
            }
        }
        return nullptr;
    }
    
    /**
     * @brief 获取所有指定类型的组件
     * @tparam T 组件类型
     * @return 组件指针列表
     */
    template<typename T>
    std::vector<T*> getComponents() const {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        
        std::vector<T*> result;
        for (const auto& component : m_components) {
            if (T* ptr = dynamic_cast<T*>(component.get())) {
                result.push_back(ptr);
            }
        }
        return result;
    }
    
    /**
     * @brief 移除组件
     * @tparam T 组件类型
     */
    template<typename T>
    void removeComponent() {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        
        m_components.erase(
            std::remove_if(m_components.begin(), m_components.end(),
                [](const std::unique_ptr<Component>& comp) {
                    return dynamic_cast<T*>(comp.get()) != nullptr;
                }),
            m_components.end()
        );
    }
    
    /**
     * @brief 检查是否有指定类型的组件
     * @tparam T 组件类型
     */
    template<typename T>
    bool hasComponent() const {
        return getComponent<T>() != nullptr;
    }

    // ===== 层级管理 =====
    
    /**
     * @brief 设置父对象
     * @param parent 新的父对象（nullptr 表示移到根级）
     */
    void setParent(GameObject* parent);
    
    /**
     * @brief 获取父对象
     */
    GameObject* getParent() const { return m_parent; }
    
    /**
     * @brief 添加子对象
     * @param child 子对象（将转移所有权）
     */
    void addChild(std::unique_ptr<GameObject> child);
    
    /**
     * @brief 获取子对象数量
     */
    size_t getChildCount() const { return m_children.size(); }
    
    /**
     * @brief 获取子对象
     * @param index 索引
     */
    GameObject* getChild(size_t index) const {
        return (index < m_children.size()) ? m_children[index].get() : nullptr;
    }
    
    /**
     * @brief 通过名称查找子对象
     * @param name 子对象名称
     * @param recursive 是否递归查找
     */
    GameObject* findChild(const std::string& name, bool recursive = false) const;

    // ===== Getter/Setter =====
    
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    const std::string& getTag() const { return m_tag; }
    void setTag(const std::string& tag) { m_tag = tag; }
    
    bool isActive() const { return m_isActive; }
    void setActive(bool active) { m_isActive = active; }
    
    /**
     * @brief 检查是否在层级中激活（父对象也必须激活）
     */
    bool isActiveInHierarchy() const;
    
    Scene* getScene() const { return m_scene; }
    void setScene(Scene* scene) { m_scene = scene; }
    
    Transform* getTransform() const { return m_transform.get(); }
};

#endif // GAME_OBJECT_H
