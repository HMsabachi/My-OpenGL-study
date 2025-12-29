// Floor.cpp
#include "Floor.h"
#include <vector>
#include <memory>  // for std::shared_ptr
#include "../engine.h"

#define Ptr std::shared_ptr
#define MPtr std::make_shared

Floor::Floor(Engine* engine,
    const glm::vec3& position,
    const glm::vec3& size, Shader* shader)
    : Object(engine, position, glm::vec3(0.0f)),  // 速度为0
    m_physicsWorld(engine->pWorld),
    m_size(size),
    m_shader(shader),
    m_vao(nullptr) {

    // 初始化网格
    initMesh();

    // 创建物理静态刚体
    if (m_physicsWorld) {
        rp3d::Transform transform(rp3d::Vector3(position.x, position.y, position.z),
            rp3d::Quaternion::identity());
        m_rigidBody = m_physicsWorld->createRigidBody(transform);
        m_rigidBody->setType(rp3d::BodyType::STATIC);

        rp3d::Vector3 halfExtents(size.x / 2.0f, size.y / 2.0f, size.z / 2.0f);
        auto* boxShape = m_engine->physicsCommon.createBoxShape(halfExtents);  // 注意：需使用PhysicsCommon创建shape
        m_rigidBody->addCollider(boxShape, rp3d::Transform::identity());
    }
}

Floor::~Floor() {
    // 销毁物理刚体
    if (m_rigidBody && m_physicsWorld) {
        m_physicsWorld->destroyRigidBody(m_rigidBody);
    }

    // 释放VAO
    delete m_vao;
}

void Floor::update(float deltaTime) {
    if (m_isActive) {
        // 静态地板无需更新位置，但可同步物理如果需要
        // m_position = glm::vec3(m_rigidBody->getTransform().getPosition());
    }
}

void Floor::initMesh() {
    // 定义平面顶点 (x-z平面, y=0, 单位大小 -0.5到0.5, 带纹理坐标)
    std::vector<GLfloat> vertices = {
        -0.5f, 0.0f, -0.5f,  0.0f, 0.0f,  // 左后
         0.5f, 0.0f, -0.5f,  1.0f, 0.0f,  // 右后
         0.5f, 0.0f,  0.5f,  1.0f, 1.0f,  // 右前
        -0.5f, 0.0f,  0.5f,  0.0f, 1.0f   // 左前
    };

    std::vector<GLuint> indices = {
        0, 1, 2,  // 三角形1
        2, 3, 0   // 三角形2
    };

    auto vbo = MPtr<Buffer<float>>(vertices, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    auto ebo = MPtr<Buffer<unsigned int>>(indices, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);

    m_vao = new VAO();
    m_vao->addVBO(*vbo, "3f 2f", GL_FALSE);
    m_vao->addEBO(*ebo);
}

void Floor::render() const {
    if (!m_shader || !m_vao) return;

    // 计算模型矩阵: 平移 + 缩放 (厚度在y, 但平面在y=0)
    glm::mat4 model(1.0f);
    model = glm::translate(model, m_position);
    model = glm::scale(model, m_size);

    // 设置uniform
    m_shader->setMat4("uModel", model);

    // 绑定纹理 (假设shader已设置texture1, texture2)
    // glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, textureID);
    // 如需纹理，可添加TextureManager加载

    m_shader->begin();
    m_vao->draw();
    m_shader->end();
}

bool Floor::collideWith(const Object& other) const {
    // 简单AABB碰撞检测 (假设other为点或小物体)
    const glm::vec3& pos = other.getPosition();
    float halfW = m_size.x / 2.0f;
    float halfD = m_size.z / 2.0f;
    float top = m_position.y + m_size.y / 2.0f;

    return (pos.x >= m_position.x - halfW && pos.x <= m_position.x + halfW &&
        pos.z >= m_position.z - halfD && pos.z <= m_position.z + halfD &&
        pos.y <= top);
}