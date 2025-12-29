#include "sphere.h"
#include "../engine.h"
#include "../../wrapper/widgets.h"
#include <glm/gtc/matrix_transform.hpp>

Sphere::Sphere(Engine* engine, const glm::vec3& position, float radius, Shader* shader, GLuint texture)
    : Object(engine, position), m_radius(radius), m_shader(shader), m_texture(texture), m_indexCount(0)
{
    initMesh();
}

Sphere::~Sphere() {
    delete m_vao;
}

void Sphere::initMesh() {
    widgets::SphereData data = widgets::createSphere(m_radius, 36, 18);
    
    m_vbo = std::make_shared<Buffer<float>>(data.vertices, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    m_ebo = std::make_shared<Buffer<unsigned int>>(data.indices, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
    
    m_vao = new VAO();
    // 更新布局字符串：3f (pos) + 3f (normal) + 2f (tex)
    m_vao->addVBO(*m_vbo, "3f 3f 2f", GL_FALSE);
    m_vao->addEBO(*m_ebo);
    
    m_indexCount = data.indices.size();
}

void Sphere::update(float deltaTime) {
    // 可以在这里添加旋转逻辑或其他更新逻辑
}

void Sphere::render() const {
    if (!m_shader) return;

    m_shader->begin();
    
    // 绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    
    // 计算并设置 Model 矩阵
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_position);
    
    m_shader->setMat4("uModel", model);
    
    // 绘制
    m_vao->draw(GL_TRIANGLES, m_indexCount);
    
    m_shader->end();
}

bool Sphere::collideWith(const Object& other) const {
    // 简单的球体碰撞检测占位符
    return false;
}
