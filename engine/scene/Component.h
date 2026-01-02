// Component.h - 组件基类
#ifndef COMPONENT_H
#define COMPONENT_H

#include <string>
#include <memory>

// 前向声明
class GameObject;

/**
 * @class Component
 * @brief 组件基类 - 所有组件都必须继承此类
 * 
 * 组件模式：将功能分解为独立的可重用模块
 * 每个组件负责一个特定的功能（渲染、物理、脚本等）
 */
class Component {
protected:
    GameObject* m_owner;        // 拥有此组件的游戏对象
    bool m_enabled;             // 组件是否启用
    bool m_started;             // 是否已经调用过 start()
    std::string m_componentName; // 组件类型名称

public:
    /**
     * @brief 构造函数
     * @param owner 拥有此组件的游戏对象
     * @param name 组件类型名称
     */
    Component(GameObject* owner, const std::string& name = "Component")
        : m_owner(owner), m_enabled(true), m_started(false), m_componentName(name) {}
    
    virtual ~Component() = default;

    // ===== 生命周期方法 =====
    
    /**
     * @brief 组件被添加到对象时立即调用（用于初始化）
     */
    virtual void awake() {}
    
    /**
     * @brief 第一帧更新前调用（用于依赖其他组件的初始化）
     */
    virtual void start() {}
    
    /**
     * @brief 每帧更新
     * @param deltaTime 时间增量（秒）
     */
    virtual void update(float deltaTime) {}
    
    /**
     * @brief 固定时间步长更新（用于物理）
     * @param fixedDeltaTime 固定时间增量
     */
    virtual void fixedUpdate(float fixedDeltaTime) {}
    
    /**
     * @brief 在所有 update 之后调用（用于同步和后处理）
     * @param deltaTime 时间增量
     */
    virtual void lateUpdate(float deltaTime) {}
    
    /**
     * @brief 组件被销毁前调用（用于清理）
     */
    virtual void onDestroy() {}
    
    /**
     * @brief 当组件启用状态改变时调用
     * @param enabled 新的启用状态
     */
    virtual void onEnableChanged(bool enabled) {}

    // ===== Getter/Setter =====
    
    GameObject* getOwner() const { return m_owner; }
    
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) {
        if (m_enabled != enabled) {
            m_enabled = enabled;
            onEnableChanged(enabled);
        }
    }
    
    bool hasStarted() const { return m_started; }
    void markStarted() { m_started = true; }
    
    const std::string& getComponentName() const { return m_componentName; }
    
    /**
     * @brief 获取组件的类型标识（用于类型检查）
     */
    virtual const char* getTypeName() const { return "Component"; }
};

#endif // COMPONENT_H
