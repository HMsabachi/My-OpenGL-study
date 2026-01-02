// GameObject.cpp - 游戏对象实现
#include "GameObject.h"
#include "SceneManager.h"

GameObject::GameObject(const std::string& name, Scene* scene)
    : m_name(name),
      m_tag("Untagged"),
      m_isActive(true),
      m_scene(scene),
      m_parent(nullptr),
      m_started(false),
      m_markedForDestroy(false) {
    
    // 创建 Transform 组件（每个 GameObject 必有）
    m_transform = std::make_unique<Transform>(this);
}

GameObject::~GameObject() {
    // 调用所有组件的 onDestroy
    for (auto& component : m_components) {
        if (component) {
            component->onDestroy();
        }
    }
    
    if (m_transform) {
        m_transform->onDestroy();
    }
}

// ===== 生命周期管理 =====

void GameObject::awake() {
    if (!m_isActive) return;
    
    // 调用 Transform 的 awake
    if (m_transform) {
        m_transform->awake();
    }
    
    // 调用所有组件的 awake
    for (auto& component : m_components) {
        if (component && component->isEnabled()) {
            component->awake();
        }
    }
    
    // 递归调用子对象的 awake
    for (auto& child : m_children) {
        if (child) {
            child->awake();
        }
    }
}

void GameObject::start() {
    if (!m_isActive || m_started) return;
    
    m_started = true;
    
    // 调用 Transform 的 start
    if (m_transform && !m_transform->hasStarted()) {
        m_transform->start();
        m_transform->markStarted();
    }
    
    // 调用所有组件的 start
    for (auto& component : m_components) {
        if (component && component->isEnabled() && !component->hasStarted()) {
            component->start();
            component->markStarted();
        }
    }
    
    // 递归调用子对象的 start
    for (auto& child : m_children) {
        if (child) {
            child->start();
        }
    }
}

void GameObject::update(float deltaTime) {
    if (!m_isActive) return;
    
    // 确保已调用 start
    if (!m_started) {
        start();
    }
    
    // 调用所有组件的 update
    for (auto& component : m_components) {
        if (component && component->isEnabled()) {
            component->update(deltaTime);
        }
    }
    
    // 递归调用子对象的 update
    for (auto& child : m_children) {
        if (child) {
            child->update(deltaTime);
        }
    }
}

void GameObject::fixedUpdate(float fixedDeltaTime) {
    if (!m_isActive) return;
    
    // 调用所有组件的 fixedUpdate
    for (auto& component : m_components) {
        if (component && component->isEnabled()) {
            component->fixedUpdate(fixedDeltaTime);
        }
    }
    
    // 递归调用子对象的 fixedUpdate
    for (auto& child : m_children) {
        if (child) {
            child->fixedUpdate(fixedDeltaTime);
        }
    }
}

void GameObject::lateUpdate(float deltaTime) {
    if (!m_isActive) return;
    
    // 调用所有组件的 lateUpdate
    for (auto& component : m_components) {
        if (component && component->isEnabled()) {
            component->lateUpdate(deltaTime);
        }
    }
    
    // 递归调用子对象的 lateUpdate
    for (auto& child : m_children) {
        if (child) {
            child->lateUpdate(deltaTime);
        }
    }
}

void GameObject::destroy() {
    m_markedForDestroy = true;
    
    // 标记所有子对象为待销毁
    for (auto& child : m_children) {
        if (child) {
            child->destroy();
        }
    }
}

// ===== 层级管理 =====

void GameObject::setParent(GameObject* parent) {
    if (m_parent == parent) return;
    
    // 从旧父对象移除
    if (m_parent) {
        auto& siblings = m_parent->m_children;
        siblings.erase(
            std::remove_if(siblings.begin(), siblings.end(),
                [this](const std::unique_ptr<GameObject>& child) {
                    return child.get() == this;
                }),
            siblings.end()
        );
    }
    
    m_parent = parent;
    
    // Transform 会自动处理坐标转换
    if (m_transform) {
        m_transform->markDirty();
    }
}

void GameObject::addChild(std::unique_ptr<GameObject> child) {
    if (!child) return;
    
    child->m_parent = this;
    child->m_scene = m_scene;
    
    // 如果父对象已经启动，也启动子对象
    if (m_started && !child->m_started) {
        child->awake();
        child->start();
    }
    
    m_children.push_back(std::move(child));
}

GameObject* GameObject::findChild(const std::string& name, bool recursive) const {
    // 查找直接子对象
    for (const auto& child : m_children) {
        if (child && child->getName() == name) {
            return child.get();
        }
    }
    
    // 递归查找
    if (recursive) {
        for (const auto& child : m_children) {
            if (child) {
                if (GameObject* found = child->findChild(name, true)) {
                    return found;
                }
            }
        }
    }
    
    return nullptr;
}

bool GameObject::isActiveInHierarchy() const {
    if (!m_isActive) return false;
    
    // 检查所有父对象是否激活
    GameObject* parent = m_parent;
    while (parent) {
        if (!parent->isActive()) {
            return false;
        }
        parent = parent->getParent();
    }
    
    return true;
}
