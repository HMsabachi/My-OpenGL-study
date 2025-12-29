#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <memory>
#include <string>
#include "object/object.h"

class Engine;
class Cube;
class Sphere;
class Plane;

/**
 * @brief 场景管理类，负责管理所有游戏对象的生命周期
 */
class Scene {
public:
    /**
     * @brief 构造函数
     * @param engine 引擎指针
     */
    explicit Scene(Engine* engine);
    
    /**
     * @brief 析构函数
     */
    ~Scene();

    /**
     * @brief 添加对象到场景
     * @param object 对象指针（Scene将接管所有权）
     */
    void addObject(Object* object);

    /**
     * @brief 移除对象（通过指针）
     * @param object 要移除的对象指针
     */
    void removeObject(Object* object);

    /**
     * @brief 更新场景中所有活跃对象
     * @param deltaTime 时间增量
     */
    void update(float deltaTime);

    /**
     * @brief 渲染场景中所有活跃对象
     */
    void render() const;

    /**
     * @brief 清理所有非活跃对象
     */
    void cleanupInactiveObjects();

    /**
     * @brief 清空场景中所有对象
     */
    void clear();

    /**
     * @brief 获取场景中对象数量
     */
    size_t getObjectCount() const { return m_objects.size(); }

    /**
     * @brief 获取场景中活跃对象数量
     */
    size_t getActiveObjectCount() const;

    /**
     * @brief 按类型查找对象
     * @tparam T 对象类型
     * @return 该类型的所有对象指针列表
     */
    template<typename T>
    std::vector<T*> findObjectsByType() const {
        std::vector<T*> result;
        for (auto& obj : m_objects) {
            if (T* typed = dynamic_cast<T*>(obj.get())) {
                result.push_back(typed);
            }
        }
        return result;
    }

private:
    Engine* m_engine;                                   // 引擎指针
    std::vector<std::unique_ptr<Object>> m_objects;    // 所有对象列表
};

#endif // SCENE_H
