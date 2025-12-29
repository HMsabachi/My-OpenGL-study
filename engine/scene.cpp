#include "scene.h"
#include "engine.h"
#include "object/cube.h"
#include "object/sphere.h"
#include "object/plane.h"
#include <algorithm>

Scene::Scene(Engine* engine) 
    : m_engine(engine) {
}

Scene::~Scene() {
    // 清空对象列表，这会触发所有 unique_ptr 的析构
    // 从而调用每个 Object 的析构函数
    clear();
}

void Scene::addObject(Object* object) {
    if (object) {
        m_objects.emplace_back(object);
    }
}

void Scene::removeObject(Object* object) {
    if (!object) return;
    
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [object](const std::unique_ptr<Object>& obj) {
            return obj.get() == object;
        });
    
    if (it != m_objects.end()) {
        m_objects.erase(it);
    }
}

void Scene::update(float deltaTime) {
    // 更新物理世界
    if (m_engine && m_engine->pWorld) {
        // 固定时间步长更新物理引擎
        m_engine->pWorld->update(deltaTime);
    }
    
    // 更新所有活跃对象
    for (auto& obj : m_objects) {
        if (obj && obj->isActive()) {
            obj->update(deltaTime);
        }
    }
}

void Scene::render() const {
    // 渲染所有活跃对象
    for (const auto& obj : m_objects) {
        if (obj && obj->isActive()) {
            obj->render();
        }
    }
}

void Scene::cleanupInactiveObjects() {
    // 移除所有非活跃对象
    m_objects.erase(
        std::remove_if(m_objects.begin(), m_objects.end(),
            [](const std::unique_ptr<Object>& obj) {
                return !obj || !obj->isActive();
            }),
        m_objects.end()
    );
}

void Scene::clear() {
    // 清空对象列表，unique_ptr 会自动删除对象
    m_objects.clear();
}

size_t Scene::getActiveObjectCount() const {
    size_t count = 0;
    for (const auto& obj : m_objects) {
        if (obj && obj->isActive()) {
            ++count;
        }
    }
    return count;
}
