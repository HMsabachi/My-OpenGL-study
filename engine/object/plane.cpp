#include "plane.h"
#include "../engine.h"
#include <glm/gtc/matrix_transform.hpp>

Plane::Plane(Engine* engine, const glm::vec3& position, const glm::vec2& size, Shader* shader, GLuint texture)
    : Object(engine, position), 
      m_size(size), 
      m_shader(shader), 
      m_texture(texture),
      m_textureRepeatX(1.0f),
      m_textureRepeatZ(1.0f)
{
    initMesh();
}

Plane::~Plane() {
    delete m_vao;
}

void Plane::initMesh() {
    // 创建水平平面（XZ平面）的顶点数据
    // 格式：位置(x,y,z) + 纹理坐标(u,v)
    float halfWidth = m_size.x * 0.5f;
    float halfDepth = m_size.y * 0.5f;
    
    std::vector<float> vertices = {
        // 位置                              纹理坐标
        -halfWidth, 0.0f, -halfDepth,       0.0f, 0.0f,
         halfWidth, 0.0f, -halfDepth,       m_textureRepeatX, 0.0f,
         halfWidth, 0.0f,  halfDepth,       m_textureRepeatX, m_textureRepeatZ,
        -halfWidth, 0.0f,  halfDepth,       0.0f, m_textureRepeatZ
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,  // 第一个三角形
        2, 3, 0   // 第二个三角形
    };

    auto vbo = std::make_shared<Buffer<float>>(vertices, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    auto ebo = std::make_shared<Buffer<unsigned int>>(indices, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
    
    m_vao = new VAO();
    m_vao->addVBO(*vbo, "3f 2f", GL_FALSE);
    m_vao->addEBO(*ebo);  // 使用 addEBO 而不是 setEBO
}

void Plane::update(float deltaTime) {
    // 调用父类更新（处理物理同步）
    Object::update(deltaTime);
}

void Plane::setTextureRepeat(float repeatX, float repeatZ) {
    m_textureRepeatX = repeatX;
    m_textureRepeatZ = repeatZ;
    
    // 重新初始化网格以更新纹理坐标
    delete m_vao;
    initMesh();
}

void Plane::render() const {
    if (!m_shader) return;

    m_shader->begin();
    
    // 绑定纹理
    if (m_texture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);
    }

    // 计算并设置 Model 矩阵
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_position);
    model = model * glm::mat4_cast(m_rotation);
    model = glm::scale(model, glm::vec3(m_scale.x, 1.0f, m_scale.z));

    m_shader->setMat4("uModel", model);
    
    // 绘制
    m_vao->draw();
    
    m_shader->end();
}

bool Plane::collideWith(const Object& other) const {
    // 由物理引擎处理碰撞
    return false;
}

void Plane::applyForce(const glm::vec3& force) {
    // 平面通常是静态对象，不接受力
    // 如果有物理刚体且是动态的，可以施加力
    if (m_rigidBody && m_physicsType == PhysicsType::DYNAMIC) {
        rp3d::Vector3 rp3dForce(force.x, force.y, force.z);
        m_rigidBody->applyWorldForceAtCenterOfMass(rp3dForce);
    }
    // 静态或运动学物体不响应力
}
