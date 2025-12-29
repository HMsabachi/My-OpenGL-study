// Floor.h
#ifndef FLOOR_H
#define FLOOR_H

#include "Object.h"  // 假设Object.h已定义
#include "../glFrameWork/core.h"
#include "../glFrameWork/buffers.h"
#include "../glFrameWork/shader.h"
#include "reactphysics3d/reactphysics3d.h"

/**
 * @brief 游戏地板类，继承自Object基类。
 * 该类代表一个静态地板物体，支持物理碰撞和OpenGL渲染。
 * 使用reactphysics3d创建静态刚体，使用VAO渲染平面网格。
 */
class Floor : public Object {
public:
    /**
     * @brief 构造函数。
     * @param position 初始位置 (默认 glm::vec3(0.0f, 0.0f, 0.0f))
     * @param physicsWorld 物理世界指针 (rp3d::PhysicsWorld*)
     * @param size 地板尺寸 (glm::vec3, x:宽度, y:厚度, z:深度，默认100x1x100)
     * @param shader 着色器指针 (Shader*)
     */
    Floor(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f),
        rp3d::PhysicsWorld* physicsWorld = nullptr,
        const glm::vec3& size = glm::vec3(100.0f, 1.0f, 100.0f),
        Shader* shader = nullptr);

    /**
     * @brief 析构函数，释放资源。
     */
    ~Floor() override;

    /**
     * @brief 更新函数，静态物体无需移动，但可同步物理。
     * @param deltaTime 时间增量
     */
    void update(float deltaTime) override;

    /**
     * @brief 渲染函数，绘制地板网格。
     */
    void render() const override;

    /**
     * @brief 碰撞检测函数，使用AABB简单检查。
     * @param other 其他物体
     * @return 是否碰撞
     */
    bool collideWith(const Object& other) const override;

private:
    rp3d::PhysicsWorld* m_physicsWorld;  // 物理世界
    glm::vec3 m_size;                    // 尺寸 (宽度, 厚度, 深度)
    Shader* m_shader;                    // 着色器
    VAO* m_vao;                          // VAO指针

    /**
     * @brief 初始化网格数据 (VBO, EBO, VAO)。
     */
    void initMesh();
};

#endif // FLOOR_H