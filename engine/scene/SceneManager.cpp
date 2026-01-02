// SceneManager.cpp - 场景管理器实现
#include "SceneManager.h"
#include <algorithm>
#include <iostream>
#include <functional>

// ===== Scene 实现 =====

Scene::Scene(const std::string& name, Engine* engine)
    : m_name(name),
      m_engine(engine),
      m_started(false) {
}

Scene::~Scene() {
    clear();
}

GameObject* Scene::createGameObject(const std::string& name) {
    auto object = std::make_unique<GameObject>(name, this);
    GameObject* ptr = object.get();
    
    m_rootObjects.push_back(std::move(object));
    m_allObjects.push_back(ptr);
    
    // 如果场景已启动，立即初始化新对象
    if (m_started) {
        ptr->awake();
        ptr->start();
    }
    
    return ptr;
}

GameObject* Scene::addGameObject(std::unique_ptr<GameObject> object) {
    if (!object) return nullptr;
    
    GameObject* ptr = object.get();
    ptr->setScene(this);
    
    m_rootObjects.push_back(std::move(object));
    m_allObjects.push_back(ptr);
    
    // 如果场景已启动，立即初始化新对象
    if (m_started) {
        ptr->awake();
        ptr->start();
    }
    
    return ptr;
}

void Scene::destroyGameObject(GameObject* object) {
    if (!object) return;
    
    object->destroy();
    m_objectsToDestroy.push_back(object);
}

void Scene::destroyImmediate() {
    destroyMarkedObjects();
}

void Scene::clear() {
    m_rootObjects.clear();
    m_allObjects.clear();
    m_objectsToDestroy.clear();
    m_started = false;
}

void Scene::collectAllObjects() {
    m_allObjects.clear();
    
    // 递归收集所有对象
    std::function<void(GameObject*)> collectRecursive = [&](GameObject* obj) {
        if (!obj) return;
        
        m_allObjects.push_back(obj);
        
        for (size_t i = 0; i < obj->getChildCount(); ++i) {
            collectRecursive(obj->getChild(i));
        }
    };
    
    for (auto& rootObj : m_rootObjects) {
        collectRecursive(rootObj.get());
    }
}

void Scene::destroyMarkedObjects() {
    if (m_objectsToDestroy.empty()) return;
    
    // 从根对象列表移除
    m_rootObjects.erase(
        std::remove_if(m_rootObjects.begin(), m_rootObjects.end(),
            [this](const std::unique_ptr<GameObject>& obj) {
                return obj->isMarkedForDestroy();
            }),
        m_rootObjects.end()
    );
    
    // 从扁平列表移除
    m_allObjects.erase(
        std::remove_if(m_allObjects.begin(), m_allObjects.end(),
            [](GameObject* obj) {
                return obj->isMarkedForDestroy();
            }),
        m_allObjects.end()
    );
    
    m_objectsToDestroy.clear();
}

GameObject* Scene::findObjectByName(const std::string& name) const {
    for (GameObject* obj : m_allObjects) {
        if (obj && obj->getName() == name) {
            return obj;
        }
    }
    return nullptr;
}

GameObject* Scene::findObjectByTag(const std::string& tag) const {
    for (GameObject* obj : m_allObjects) {
        if (obj && obj->getTag() == tag) {
            return obj;
        }
    }
    return nullptr;
}

std::vector<GameObject*> Scene::findObjectsByTag(const std::string& tag) const {
    std::vector<GameObject*> result;
    
    for (GameObject* obj : m_allObjects) {
        if (obj && obj->getTag() == tag) {
            result.push_back(obj);
        }
    }
    
    return result;
}

void Scene::start() {
    if (m_started) return;
    
    m_started = true;
    
    // 收集所有对象
    collectAllObjects();
    
    // 调用所有对象的 awake
    for (GameObject* obj : m_allObjects) {
        if (obj && obj->isActive()) {
            obj->awake();
        }
    }
    
    // 调用所有对象的 start
    for (GameObject* obj : m_allObjects) {
        if (obj && obj->isActive()) {
            obj->start();
        }
    }
    
    std::cout << "[Scene] 场景 '" << m_name << "' 已启动，包含 " 
              << m_allObjects.size() << " 个对象" << std::endl;
}

void Scene::update(float deltaTime) {
    // 确保场景已启动
    if (!m_started) {
        start();
    }
    
    // 更新所有根对象（会递归更新子对象）
    for (auto& obj : m_rootObjects) {
        if (obj && obj->isActive()) {
            obj->update(deltaTime);
        }
    }
    
    // 销毁标记的对象
    destroyMarkedObjects();
}

void Scene::fixedUpdate(float fixedDeltaTime) {
    for (auto& obj : m_rootObjects) {
        if (obj && obj->isActive()) {
            obj->fixedUpdate(fixedDeltaTime);
        }
    }
}

void Scene::lateUpdate(float deltaTime) {
    for (auto& obj : m_rootObjects) {
        if (obj && obj->isActive()) {
            obj->lateUpdate(deltaTime);
        }
    }
}

// ===== SceneManager 实现 =====

SceneManager::SceneManager(Engine* engine)
    : m_engine(engine),
      m_activeScene(nullptr) {
}

Scene* SceneManager::createScene(const std::string& name) {
    // 检查是否已存在同名场景
    if (m_scenes.find(name) != m_scenes.end()) {
        std::cerr << "[SceneManager] 场景 '" << name << "' 已存在" << std::endl;
        return m_scenes[name].get();
    }
    
    auto scene = std::make_unique<Scene>(name, m_engine);
    Scene* ptr = scene.get();
    
    m_scenes[name] = std::move(scene);
    
    // 如果没有活跃场景，设置为活跃
    if (!m_activeScene) {
        m_activeScene = ptr;
    }
    
    std::cout << "[SceneManager] 创建场景 '" << name << "'" << std::endl;
    
    return ptr;
}

bool SceneManager::loadScene(const std::string& name) {
    auto it = m_scenes.find(name);
    if (it == m_scenes.end()) {
        std::cerr << "[SceneManager] 场景 '" << name << "' 不存在" << std::endl;
        return false;
    }
    
    m_activeScene = it->second.get();
    
    // 启动场景（如果尚未启动）
    if (m_activeScene) {
        m_activeScene->start();
    }
    
    std::cout << "[SceneManager] 加载场景 '" << name << "'" << std::endl;
    
    return true;
}

void SceneManager::unloadScene(const std::string& name) {
    auto it = m_scenes.find(name);
    if (it == m_scenes.end()) {
        return;
    }
    
    // 如果是活跃场景，清空活跃场景指针
    if (m_activeScene == it->second.get()) {
        m_activeScene = nullptr;
    }
    
    m_scenes.erase(it);
    
    std::cout << "[SceneManager] 卸载场景 '" << name << "'" << std::endl;
}

Scene* SceneManager::getScene(const std::string& name) const {
    auto it = m_scenes.find(name);
    return (it != m_scenes.end()) ? it->second.get() : nullptr;
}

void SceneManager::update(float deltaTime) {
    if (m_activeScene) {
        m_activeScene->update(deltaTime);
    }
}

void SceneManager::fixedUpdate(float fixedDeltaTime) {
    if (m_activeScene) {
        m_activeScene->fixedUpdate(fixedDeltaTime);
    }
}

void SceneManager::lateUpdate(float deltaTime) {
    if (m_activeScene) {
        m_activeScene->lateUpdate(deltaTime);
    }
}
