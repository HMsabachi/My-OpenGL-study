#include "cube.h"
#include "../engine.h"
#include <glm/gtc/matrix_transform.hpp>

Cube::Cube(Engine* engine, const glm::vec3& position, const glm::vec3& size, Shader* shader, GLuint texture1, GLuint texture2)
    : Object(engine, position), m_size(size), m_shader(shader), m_texture1(texture1), m_texture2(texture2),
      m_rotationAngle(0.0f), m_rotationAxis(glm::vec3(1.0f, 0.3f, 0.5f))
{
    // 设置缩放
    setScale(size);
    initMesh();
}

Cube::~Cube() {
    delete m_vao;
}

void Cube::initMesh() {
    std::vector<float> vertices = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    auto Vbo = std::make_shared<Buffer<float>>(vertices, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    m_vao = new VAO();
    m_vao->addVBO(*Vbo, "3f 2f", GL_FALSE);
}

void Cube::update(float deltaTime) {
    // 调用父类更新（处理物理同步）
    Object::update(deltaTime);
    
    // 可以在这里添加特定于Cube的更新逻辑
}

void Cube::setRotation(float angle, const glm::vec3& axis) {
    m_rotationAngle = angle;
    m_rotationAxis = axis;
    
    // 将欧拉角旋转转换为四元数并设置到父类
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis);
    Object::setRotation(glm::quat_cast(rotationMatrix));
}

void Cube::render() const {
    if (!m_shader) return;

    m_shader->begin();
    
    // 绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture1);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_texture2);

    // 计算并设置 Model 矩阵，使用父类的变换信息
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_position);
    model = model * glm::mat4_cast(m_rotation);  // 使用四元数旋转
    model = glm::scale(model, m_scale);

    m_shader->setMat4("uModel", model);
    
    // 绘制
    m_vao->draw();
    
    m_shader->end();
}

bool Cube::collideWith(const Object& other) const {
    // 简单的AABB碰撞检测占位符
    // 如果使用物理引擎，由物理引擎处理碰撞
    return false;
}

void Cube::applyForce(const glm::vec3& force) {
    // 如果有物理刚体，施加力
    if (m_rigidBody) {
        rp3d::Vector3 rp3dForce(force.x, force.y, force.z);
        m_rigidBody->applyWorldForceAtCenterOfMass(rp3dForce);
    }
    // 如果没有物理体，什么也不做（或者可以直接修改速度）
}
