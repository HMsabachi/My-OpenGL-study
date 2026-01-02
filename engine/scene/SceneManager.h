// SceneManager.h - 场景管理器
#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "GameObject.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

class Engine;

/**
 * @class Scene
 * @brief 场景类 - 管理游戏对象的容器
 * 
 * 场景包含所有游戏对象及其层级关系
 * 负责对象的生命周期管理和更新调度
 */
class Scene {
private:
    std::string m_name;                                      // 场景名称
    Engine* m_engine;                                        // 引擎引用
    
    std::vector<std::unique_ptr<GameObject>> m_rootObjects;  // 根级游戏对象
    std::vector<GameObject*> m_allObjects;                   // 所有对象的扁平列表（用于快速查找）
    
    bool m_started;                                          // 场景是否已启动
    
    // 待销毁对象列表（延迟销毁，避免迭代中删除）
    std::vector<GameObject*> m_objectsToDestroy;
    
    // 辅助方法
    void collectAllObjects();                                // 收集所有对象到扁平列表
    void destroyMarkedObjects();                             // 销毁标记为待销毁的对象

public:
    /**
     * @brief 构造函数
     * @param name 场景名称
     * @param engine 引擎指针
     */
    Scene(const std::string& name, Engine* engine);
    
    ~Scene();

    // ===== 对象管理 =====
    
    /**
     * @brief 创建游戏对象
     * @param name 对象名称
     * @return 指向新对象的指针
     */
    GameObject* createGameObject(const std::string& name = "GameObject");
    
    /**
     * @brief 添加游戏对象到场景
     * @param object 对象（转移所有权）
     * @return 指向对象的指针
     */
    GameObject* addGameObject(std::unique_ptr<GameObject> object);
    
    /**
     * @brief 销毁游戏对象（延迟销毁）
     * @param object 要销毁的对象
     */
    void destroyGameObject(GameObject* object);
    
    /**
     * @brief 立即销毁所有标记为待销毁的对象
     */
    void destroyImmediate();
    
    /**
     * @brief 清空场景（销毁所有对象）
     */
    void clear();

    // ===== 查找对象 =====
    
    /**
     * @brief 通过名称查找对象
     * @param name 对象名称
     * @return 找到的对象指针，未找到返回 nullptr
     */
    GameObject* findObjectByName(const std::string& name) const;
    
    /**
     * @brief 通过标签查找对象
     * @param tag 对象标签
     * @return 找到的第一个对象指针，未找到返回 nullptr
     */
    GameObject* findObjectByTag(const std::string& tag) const;
    
    /**
     * @brief 通过标签查找所有对象
     * @param tag 对象标签
     * @return 找到的对象列表
     */
    std::vector<GameObject*> findObjectsByTag(const std::string& tag) const;
    
    /**
     * @brief 查找包含指定组件的所有对象
     * @tparam T 组件类型
     * @return 对象列表
     */
    template<typename T>
    std::vector<GameObject*> findObjectsWithComponent() const {
        std::vector<GameObject*> result;
        
        for (GameObject* obj : m_allObjects) {
            if (obj && obj->getComponent<T>()) {
                result.push_back(obj);
            }
        }
        
        return result;
    }

    // ===== 生命周期 =====
    
    /**
     * @brief 场景启动（调用所有对象的 awake 和 start）
     */
    void start();
    
    /**
     * @brief 更新场景
     * @param deltaTime 时间增量
     */
    void update(float deltaTime);
    
    /**
     * @brief 固定更新（用于物理）
     * @param fixedDeltaTime 固定时间增量
     */
    void fixedUpdate(float fixedDeltaTime);
    
    /**
     * @brief 延迟更新
     * @param deltaTime 时间增量
     */
    void lateUpdate(float deltaTime);

    // ===== Getter =====
    
    const std::string& getName() const { return m_name; }
    Engine* getEngine() const { return m_engine; }
    
    size_t getRootObjectCount() const { return m_rootObjects.size(); }
    size_t getTotalObjectCount() const { return m_allObjects.size(); }
    
    /**
     * @brief 获取根级对象
     * @param index 索引
     */
    GameObject* getRootObject(size_t index) const {
        return (index < m_rootObjects.size()) ? m_rootObjects[index].get() : nullptr;
    }
};

/**
 * @class SceneManager
 * @brief 场景管理器 - 管理多个场景的加载和切换
 */
class SceneManager {
private:
    Engine* m_engine;
    std::unordered_map<std::string, std::unique_ptr<Scene>> m_scenes;
    Scene* m_activeScene;

public:
    /**
     * @brief 构造函数
     * @param engine 引擎指针
     */
    SceneManager(Engine* engine);
    
    ~SceneManager() = default;

    /**
     * @brief 创建新场景
     * @param name 场景名称
     * @return 指向新场景的指针
     */
    Scene* createScene(const std::string& name);
    
    /**
     * @brief 加载场景（设置为活跃场景）
     * @param name 场景名称
     * @return 是否成功
     */
    bool loadScene(const std::string& name);
    
    /**
     * @brief 卸载场景
     * @param name 场景名称
     */
    void unloadScene(const std::string& name);
    
    /**
     * @brief 获取活跃场景
     */
    Scene* getActiveScene() const { return m_activeScene; }
    
    /**
     * @brief 获取指定场景
     * @param name 场景名称
     */
    Scene* getScene(const std::string& name) const;
    
    /**
     * @brief 更新活跃场景
     * @param deltaTime 时间增量
     */
    void update(float deltaTime);
    
    /**
     * @brief 固定更新活跃场景
     * @param fixedDeltaTime 固定时间增量
     */
    void fixedUpdate(float fixedDeltaTime);
    
    /**
     * @brief 延迟更新活跃场景
     * @param deltaTime 时间增量
     */
    void lateUpdate(float deltaTime);
};

#endif // SCENE_MANAGER_H
