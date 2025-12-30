#include "slime.h"
#include "../engine.h"
#include "../../wrapper/widgets.h"
#include <glm/gtc/matrix_transform.hpp>
#include <random>

Slime::Slime(Engine* engine, const glm::vec3& center, int numParticles, 
             float particleRadius, float initialRadius, Shader* shader, GLuint texture)
    : Object(engine, center),
      m_center(center),
      m_numParticles(numParticles),
      m_particleRadius(particleRadius),
      m_shader(shader),
      m_texture(texture),
      m_particleVAO(nullptr),
      m_indexCount(0),
      m_sphereShape(nullptr),
      m_cohesionForce(10.0f),    // 向心力强度
      m_damping(0.98f)            // 阻尼系数
{
    // 初始化粒子网格
    initMesh();
    
    // 初始化粒子
    initParticles(initialRadius);
}

Slime::~Slime() {
    cleanupPhysics();
    delete m_particleVAO;
}

void Slime::initMesh() {
    // 创建单个球体网格（所有粒子共享）
    widgets::SphereData data = widgets::createSphere(m_particleRadius, 16, 8);
    
    m_vbo = std::make_shared<Buffer<float>>(data.vertices, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    m_ebo = std::make_shared<Buffer<unsigned int>>(data.indices, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
    
    m_particleVAO = new VAO();
    // 布局：3f (pos) + 3f (normal) + 2f (tex)
    m_particleVAO->addVBO(*m_vbo, "3f 3f 2f", GL_FALSE);
    m_particleVAO->addEBO(*m_ebo);
    
    m_indexCount = data.indices.size();
}

void Slime::initParticles(float initialRadius) {
    if (!m_engine || !m_engine->pWorld) return;

    // 创建共享的球体碰撞形状
    m_sphereShape = m_engine->physicsCommon.createSphereShape(m_particleRadius);
    
    // 随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    
    m_particles.resize(m_numParticles);
    
    for (int i = 0; i < m_numParticles; ++i) {
        Particle& p = m_particles[i];
        
        // 在球体内随机生成初始位置
        glm::vec3 randomOffset;
        do {
            randomOffset = glm::vec3(dis(gen), dis(gen), dis(gen));
        } while (glm::length(randomOffset) > 1.0f);  // 确保在单位球内
        
        randomOffset *= initialRadius;
        p.position = m_center + randomOffset;
        p.velocity = glm::vec3(0.0f);
        
        // 创建物理刚体
        rp3d::Vector3 rp3dPos(p.position.x, p.position.y, p.position.z);
        rp3d::Transform transform(rp3dPos, rp3d::Quaternion::identity());
        
        p.rigidBody = m_engine->pWorld->createRigidBody(transform);
        p.rigidBody->setType(rp3d::BodyType::DYNAMIC);
        
        // 添加碰撞体
        rp3d::Transform colliderTransform = rp3d::Transform::identity();
        p.collider = p.rigidBody->addCollider(m_sphereShape, colliderTransform);
        
        // 设置质量和物理属性
        p.collider->getMaterial().setMassDensity(1.0f);
        p.collider->getMaterial().setBounciness(0.3f);  // 弹性
        p.collider->getMaterial().setFrictionCoefficient(0.5f);  // 摩擦力
        p.rigidBody->updateMassPropertiesFromColliders();
        
        // 启用重力
        p.rigidBody->enableGravity(true);
    }
}

void Slime::cleanupPhysics() {
    if (!m_engine || !m_engine->pWorld) return;

    // 清理所有粒子的物理体
    for (auto& particle : m_particles) {
        if (particle.rigidBody) {
            if (particle.collider) {
                particle.rigidBody->removeCollider(particle.collider);
                particle.collider = nullptr;
            }
            m_engine->pWorld->destroyRigidBody(particle.rigidBody);
            particle.rigidBody = nullptr;
        }
    }
    
    // 清理共享的碰撞形状
    if (m_sphereShape) {
        m_engine->physicsCommon.destroySphereShape(static_cast<rp3d::SphereShape*>(m_sphereShape));
        m_sphereShape = nullptr;
    }
}

void Slime::update(float deltaTime) {
    if (!m_isActive) return;

    // 1. 从物理引擎同步粒子位置
    syncParticlesFromPhysics();
    
    // 2. 更新中心点
    updateCenter();
    
    // 3. 应用向心力
    applyForces(deltaTime);
    
    // 更新父类位置为中心点
    m_position = m_center;
}

void Slime::syncParticlesFromPhysics() {
    for (auto& particle : m_particles) {
        if (particle.rigidBody) {
            const rp3d::Transform& transform = particle.rigidBody->getTransform();
            const rp3d::Vector3& pos = transform.getPosition();
            particle.position = glm::vec3(pos.x, pos.y, pos.z);
            
            // 获取速度
            const rp3d::Vector3& vel = particle.rigidBody->getLinearVelocity();
            particle.velocity = glm::vec3(vel.x, vel.y, vel.z);
        }
    }
}

void Slime::updateCenter() {
    // 计算所有粒子的平均位置作为中心点
    glm::vec3 sum(0.0f);
    for (const auto& particle : m_particles) {
        sum += particle.position;
    }
    m_center = sum / static_cast<float>(m_particles.size());
}

void Slime::applyForces(float deltaTime) {
    for (auto& particle : m_particles) {
        if (!particle.rigidBody) continue;
        
        // 计算指向中心的向心力
        glm::vec3 toCenter = m_center - particle.position;
        float distance = glm::length(toCenter);
        
        if (distance > 0.001f) {  // 避免除零
            glm::vec3 cohesionDirection = glm::normalize(toCenter);
            
            // 向心力：距离越远，力越大
            glm::vec3 cohesionForceVec = cohesionDirection * m_cohesionForce * distance;
            
            // 应用力到刚体（使用正确的 API）
            rp3d::Vector3 rp3dForce(cohesionForceVec.x, cohesionForceVec.y, cohesionForceVec.z);
            particle.rigidBody->applyWorldForceAtCenterOfMass(rp3dForce);
        }
        
        // 应用阻尼（减速）
        rp3d::Vector3 currentVel = particle.rigidBody->getLinearVelocity();
        particle.rigidBody->setLinearVelocity(currentVel * m_damping);
    }
}

void Slime::render() const {
    if (!m_shader || !m_particleVAO) return;

    m_shader->begin();
    
    // 绑定纹理
    if (m_texture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);
    }
    
    // 渲染每个粒子
    for (const auto& particle : m_particles) {
        // 为每个粒子创建模型矩阵
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, particle.position);
        // 粒子本身已经有半径了，所以缩放为1
        model = glm::scale(model, glm::vec3(1.0f));
        
        m_shader->setMat4("uModel", model);
        
        // 绘制
        m_particleVAO->draw(GL_TRIANGLES, m_indexCount);
    }
    
    m_shader->end();
}

bool Slime::collideWith(const Object& other) const {
    // 碰撞由物理引擎处理
    return false;
}
