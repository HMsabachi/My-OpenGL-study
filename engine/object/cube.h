#ifndef CUBE_H
#define CUBE_H

#include "Object.h"
#include "../../glFrameWork/core.h"
#include "../../glFrameWork/buffers.h"
#include "../../glFrameWork/shader.h"
#include "../../glFrameWork/texture.h"

class Cube : public Object {
public:
    Cube(Engine* engine,
         const glm::vec3& position = glm::vec3(0.0f),
         const glm::vec3& size = glm::vec3(1.0f),
         Shader* shader = nullptr,
         GLuint texture1 = 0,
         GLuint texture2 = 0);

    ~Cube() override;

    void update(float deltaTime) override;
    void render() const override;
    bool collideWith(const Object& other) const override;

    void setRotation(float angle, const glm::vec3& axis);
    
    // 实现 applyForce
    void applyForce(const glm::vec3& force) override;

private:
    glm::vec3 m_size;
    Shader* m_shader;
    VAO* m_vao;
    GLuint m_texture1;
    GLuint m_texture2;
    
    float m_rotationAngle;
    glm::vec3 m_rotationAxis;

    void initMesh();
};

#endif // CUBE_H
