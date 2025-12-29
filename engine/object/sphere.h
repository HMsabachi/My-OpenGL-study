#ifndef SPHERE_H
#define SPHERE_H

#include "Object.h"
#include "../../glFrameWork/core.h"
#include "../../glFrameWork/buffers.h"
#include "../../glFrameWork/shader.h"
#include "../../glFrameWork/texture.h"

class Sphere : public Object {
public:
    Sphere(Engine* engine,
           const glm::vec3& position = glm::vec3(0.0f),
           float radius = 1.0f,
           Shader* shader = nullptr,
           GLuint texture = 0);

    ~Sphere() override;

    void update(float deltaTime) override;
    void render() const override;
    bool collideWith(const Object& other) const override;

private:
    float m_radius;
    Shader* m_shader;
    VAO* m_vao;
    GLuint m_texture;
    size_t m_indexCount;

    // Keep buffers alive
    std::shared_ptr<Buffer<float>> m_vbo;
    std::shared_ptr<Buffer<unsigned int>> m_ebo;

    void initMesh();
};

#endif // SPHERE_H
