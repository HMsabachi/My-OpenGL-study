#ifndef PLANE_H
#define PLANE_H

#include "Object.h"
#include "../../glFrameWork/core.h"
#include "../../glFrameWork/buffers.h"
#include "../../glFrameWork/shader.h"
#include "../../glFrameWork/texture.h"

/**
 * @brief 平面类，用于地板、墙壁等
 */
class Plane : public Object {
public:
    /**
     * @brief 构造函数
     * @param engine 引擎指针
     * @param position 位置
     * @param size 平面尺寸（x, z方向）
     * @param shader 着色器
     * @param texture 纹理
     */
    Plane(Engine* engine,
          const glm::vec3& position = glm::vec3(0.0f),
          const glm::vec2& size = glm::vec2(10.0f, 10.0f),
          Shader* shader = nullptr,
          GLuint texture = 0);

    ~Plane() override;

    void update(float deltaTime) override;
    void render() const override;
    bool collideWith(const Object& other) const override;

    /**
     * @brief 设置纹理重复次数
     */
    void setTextureRepeat(float repeatX, float repeatZ);
    
    // 实现 applyForce
    void applyForce(const glm::vec3& force) override;

private:
    glm::vec2 m_size;           // 平面尺寸
    Shader* m_shader;           // 着色器
    VAO* m_vao;                 // VAO
    GLuint m_texture;           // 纹理
    float m_textureRepeatX;     // 纹理X方向重复
    float m_textureRepeatZ;     // 纹理Z方向重复

    void initMesh();
};

#endif // PLANE_H
