#include "slime.h"
#include "../engine.h"
#include "../../wrapper/widgets.h"
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <iostream>

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
      m_damping(0.98f),          // 阻尼系数
      m_coreParticleIndex(-1),   // ✅ 初始化中心粒子索引
      m_forceRadius(1.5f),       // ✅ 默认力作用半径
      m_verticalBias(2.0f),      // ✅ 默认垂直偏好权重
      m_gravityBoost(5.0f),      // ✅ 默认额外重力强度
      // ✅ 初始化新参数
      m_repulsionForce(20.0f),           // 排斥力强度（防止过度聚集）
      m_repulsionRadius(0.18f),          // 排斥力半径（略大于粒子直径）
      m_surfaceTension(2.0f),            // 表面张力
      m_viscosity(0.5f),                 // 流体粘度
      m_restDensity(0.16f)               // 静止密度（目标粒子间距）
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
    
    // ✅ 改进的初始化：确保粒子均匀分布，不重叠
    int particlesCreated = 0;
    int maxAttempts = m_numParticles * 10;
    int attempts = 0;
    
    while (particlesCreated < m_numParticles && attempts < maxAttempts) {
        attempts++;
        
        // 在球体内随机生成位置
        glm::vec3 randomOffset;
        do {
            randomOffset = glm::vec3(dis(gen), dis(gen), dis(gen));
        } while (glm::length(randomOffset) > 1.0f);
        
        randomOffset *= initialRadius;
        glm::vec3 newPosition = m_center + randomOffset;
        
        // ✅ 检查是否与已有粒子重叠
        bool tooClose = false;
        float minDistance = m_particleRadius * 2.1f;
        
        for (int j = 0; j < particlesCreated; ++j) {
            float dist = glm::length(m_particles[j].position - newPosition);
            if (dist < minDistance) {
                tooClose = true;
                break;
            }
        }
        
        // 如果不重叠，创建粒子
        if (!tooClose) {
            Particle& p = m_particles[particlesCreated];
            p.position = newPosition;
            p.velocity = glm::vec3(0.0f);
            
            // 创建物理刚体
            rp3d::Vector3 rp3dPos(p.position.x, p.position.y, p.position.z);
            rp3d::Transform transform(rp3dPos, rp3d::Quaternion::identity());
            
            p.rigidBody = m_engine->pWorld->createRigidBody(transform);
            p.rigidBody->setType(rp3d::BodyType::DYNAMIC);
            
            // 添加碰撞体
            rp3d::Transform colliderTransform = rp3d::Transform::identity();
            p.collider = p.rigidBody->addCollider(m_sphereShape, colliderTransform);
            
            // ✅ 优化物理属性 - 极低摩擦力和高弹性
            p.collider->getMaterial().setMassDensity(0.8f);        // 降低密度，更轻盈
            p.collider->getMaterial().setBounciness(0.7f);         // 提高弹性
            p.collider->getMaterial().setFrictionCoefficient(0.01f); // 极低摩擦
            p.rigidBody->updateMassPropertiesFromColliders();
            
            // 启用重力
            p.rigidBody->enableGravity(true);
            
            // ✅ 降低线性阻尼（空气阻力）
            p.rigidBody->setLinearDamping(0.05f);  // 从 0.1 降到 0.05
            
            particlesCreated++;
        }
    }
    
    // 如果无法创建足够的粒子，调整数组大小
    if (particlesCreated < m_numParticles) {
        std::cout << "警告：只创建了 " << particlesCreated << " / " << m_numParticles 
                  << " 个粒子（空间不足或半径太大）" << std::endl;
        m_particles.resize(particlesCreated);
        m_numParticles = particlesCreated;
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
    
    // 2. 更新几何中心（质心）
    updateCenter();
    
    // 3. 更新中心粒子（动态选择最靠近质心的粒子）
    updateCoreParticle();
    
    // 4. ✅ 应用多种力（按顺序）
    applyRepulsionForces(deltaTime);    // 排斥力（防止过度聚集）
    applyForces(deltaTime);             // 向心力（维持形态）
    applySurfaceTension(deltaTime);     // 表面张力（表面粒子内聚）
    applyViscosity(deltaTime);          // 粘度（速度同步）
    
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
    // 计算所有粒子的平均位置作为几何中心（质心）
    glm::vec3 sum(0.0f);
    for (const auto& particle : m_particles) {
        sum += particle.position;
    }
    m_center = sum / static_cast<float>(m_particles.size());
}

void Slime::updateCoreParticle() {
    // 找到距离质心最近且偏向下方的粒子，作为中心粒子
    if (m_particles.empty()) {
        m_coreParticleIndex = -1;
        return;
    }
    
    float minScore = std::numeric_limits<float>::max();
    int bestIndex = 0;
    
    for (size_t i = 0; i < m_particles.size(); ++i) {
        const glm::vec3& particlePos = m_particles[i].position;
        
        // 计算水平距离（X-Z平面）
        glm::vec2 horizontalOffset(particlePos.x - m_center.x, particlePos.z - m_center.z);
        float horizontalDistance = glm::length(horizontalOffset);
        
        // 计算垂直偏移（Y轴）
        float verticalOffset = particlePos.y - m_center.y;
        
        // 权重系数
        float verticalPenalty = verticalOffset * m_verticalBias;
        
        float score = horizontalDistance + verticalPenalty;
        
        if (score < minScore) {
            minScore = score;
            bestIndex = static_cast<int>(i);
        }
    }
    
    // 更新核心粒子标记
    if (m_coreParticleIndex != -1 && m_coreParticleIndex < static_cast<int>(m_particles.size())) {
        m_particles[m_coreParticleIndex].isCore = false;
    }
    
    m_coreParticleIndex = bestIndex;
    m_particles[m_coreParticleIndex].isCore = true;
}

void Slime::applyForces(float deltaTime) {
    float maxCohesionDistance = this->m_maxCohesionDistance;
    
    // 时间独立的力缩放因子
    const float targetDeltaTime = 1.0f / 60.0f;
    float timeScale = deltaTime / targetDeltaTime;
    timeScale = glm::clamp(timeScale, 0.1f, 3.0f);
    
    for (auto& particle : m_particles) {
        if (!particle.rigidBody) continue;
        
        // ✅ 优化向心力计算：使用平方根衰减代替线性衰减
        glm::vec3 toCenter = m_center - particle.position;
        float distance = glm::length(toCenter);
        
        // 只在一定距离内施加向心力
        if (distance > 0.001f && distance < maxCohesionDistance) {
            glm::vec3 cohesionDirection = glm::normalize(toCenter);
            
            // ✅ 改进的力计算：使用平方根衰减，避免近距离力度过大
            float normalizedDistance = distance / maxCohesionDistance;
            float forceMagnitude = m_cohesionForce * glm::sqrt(normalizedDistance);
            
            glm::vec3 cohesionForceVec = cohesionDirection * forceMagnitude * timeScale;
            
            rp3d::Vector3 rp3dForce(cohesionForceVec.x, cohesionForceVec.y, cohesionForceVec.z);
            particle.rigidBody->applyWorldForceAtCenterOfMass(rp3dForce);
        }
        
        // 如果粒子距离中心太远，强制拉回
        const float emergencyDistance = maxCohesionDistance * 2.7f;
        if (distance > emergencyDistance) {
            glm::vec3 emergencyDirection = glm::normalize(toCenter);
            float emergencyForceMagnitude = m_cohesionForce * 10.0f * timeScale;
            glm::vec3 emergencyForce = emergencyDirection * emergencyForceMagnitude;
            
            rp3d::Vector3 rp3dEmergencyForce(emergencyForce.x, emergencyForce.y, emergencyForce.z);
            particle.rigidBody->applyWorldForceAtCenterOfMass(rp3dEmergencyForce);
            
            rp3d::Vector3 currentVel = particle.rigidBody->getLinearVelocity();
            particle.rigidBody->setLinearVelocity(currentVel * 0.3f);
        }

        // ✅ 降低阻尼强度，增加流动性
        rp3d::Vector3 currentVel = particle.rigidBody->getLinearVelocity();
        glm::vec3 vel(currentVel.x, currentVel.y, currentVel.z);
        glm::vec3 horizontalVel(vel.x, 0.0f, vel.z);
        float verticalVel = vel.y;
        
        // ✅ 降低水平阻尼（从 0.98 改为 0.99）
        float frameDamping = glm::pow(0.99f, deltaTime * 60.0f);
        horizontalVel *= frameDamping;
        
        // 垂直方向阻尼
        float verticalDamping = glm::pow(0.995f, deltaTime * 60.0f);
        verticalVel *= verticalDamping;
        
        glm::vec3 newVel(horizontalVel.x, verticalVel, horizontalVel.z);
        particle.rigidBody->setLinearVelocity(rp3d::Vector3(newVel.x, newVel.y, newVel.z));
    }
}

// ✅ 新增方法：应用粒子间排斥力（防止过度聚集）
void Slime::applyRepulsionForces(float deltaTime) {
    const float targetDeltaTime = 1.0f / 60.0f;
    float timeScale = deltaTime / targetDeltaTime;
    timeScale = glm::clamp(timeScale, 0.1f, 3.0f);
    
    // 使用简单的 O(n²) 算法（对于几百个粒子足够快）
    for (size_t i = 0; i < m_particles.size(); ++i) {
        auto& p1 = m_particles[i];
        if (!p1.rigidBody) continue;
        
        glm::vec3 totalRepulsion(0.0f);
        
        for (size_t j = i + 1; j < m_particles.size(); ++j) {
            auto& p2 = m_particles[j];
            if (!p2.rigidBody) continue;
            
            glm::vec3 diff = p1.position - p2.position;
            float distance = glm::length(diff);
            
            // 只有距离小于排斥半径时才施加排斥力
            if (distance > 0.001f && distance < m_repulsionRadius) {
                glm::vec3 direction = glm::normalize(diff);
                
                // 距离越近，排斥力越强（指数衰减）
                float repulsionStrength = m_repulsionForce * glm::pow((m_repulsionRadius - distance) / m_repulsionRadius, 2.0f);
                
                glm::vec3 repulsionForce = direction * repulsionStrength * timeScale;
                
                // 牛顿第三定律：作用力与反作用力
                rp3d::Vector3 force1(repulsionForce.x, repulsionForce.y, repulsionForce.z);
                rp3d::Vector3 force2(-repulsionForce.x, -repulsionForce.y, -repulsionForce.z);
                
                p1.rigidBody->applyWorldForceAtCenterOfMass(force1);
                p2.rigidBody->applyWorldForceAtCenterOfMass(force2);
            }
        }
    }
}

// ✅ 新增方法：应用表面张力（表面粒子相互吸引）
void Slime::applySurfaceTension(float deltaTime) {
    const float targetDeltaTime = 1.0f / 60.0f;
    float timeScale = deltaTime / targetDeltaTime;
    timeScale = glm::clamp(timeScale, 0.1f, 3.0f);
    
    for (auto& particle : m_particles) {
        if (!particle.rigidBody) continue;
        
        // 计算局部密度（周围粒子数量）
        int neighborCount = 0;
        glm::vec3 avgNeighborPos(0.0f);
        
        for (const auto& other : m_particles) {
            if (&other == &particle) continue;
            
            float distance = glm::length(other.position - particle.position);
            if (distance < m_repulsionRadius * 2.0f) {
                neighborCount++;
                avgNeighborPos += other.position;
            }
        }
        
        // 如果是表面粒子（邻居少），施加表面张力
        if (neighborCount > 0 && neighborCount < 8) {  // 邻居数阈值
            avgNeighborPos /= static_cast<float>(neighborCount);
            
            glm::vec3 toAvg = avgNeighborPos - particle.position;
            float distance = glm::length(toAvg);
            
            if (distance > 0.001f) {
                glm::vec3 tensionForce = glm::normalize(toAvg) * m_surfaceTension * timeScale;
                
                rp3d::Vector3 rp3dForce(tensionForce.x, tensionForce.y, tensionForce.z);
                particle.rigidBody->applyWorldForceAtCenterOfMass(rp3dForce);
            }
        }
    }
}

// ✅ 新增方法：应用流体粘度（速度同步）
void Slime::applyViscosity(float deltaTime) {
    const float targetDeltaTime = 1.0f / 60.0f;
    float timeScale = deltaTime / targetDeltaTime;
    timeScale = glm::clamp(timeScale, 0.1f, 3.0f);
    
    for (auto& particle : m_particles) {
        if (!particle.rigidBody) continue;
        
        glm::vec3 avgVelocity(0.0f);
        int neighborCount = 0;
        
        // 计算邻居的平均速度
        for (const auto& other : m_particles) {
            if (&other == &particle) continue;
            
            float distance = glm::length(other.position - particle.position);
            if (distance < m_repulsionRadius * 2.5f) {
                avgVelocity += other.velocity;
                neighborCount++;
            }
        }
        
        if (neighborCount > 0) {
            avgVelocity /= static_cast<float>(neighborCount);
            
            // 向平均速度靠拢（粘度效应）
            glm::vec3 velocityDiff = avgVelocity - particle.velocity;
            glm::vec3 viscosityForce = velocityDiff * m_viscosity * timeScale;
            
            rp3d::Vector3 rp3dForce(viscosityForce.x, viscosityForce.y, viscosityForce.z);
            particle.rigidBody->applyWorldForceAtCenterOfMass(rp3dForce);
        }
    }
}

// ✅ 新增辅助方法：计算局部密度
float Slime::calculateLocalDensity(int particleIndex) {
    if (particleIndex < 0 || particleIndex >= static_cast<int>(m_particles.size())) {
        return 0.0f;
    }
    
    const glm::vec3& pos = m_particles[particleIndex].position;
    int neighborCount = 0;
    
    for (const auto& other : m_particles) {
        float distance = glm::length(other.position - pos);
        if (distance > 0.001f && distance < m_repulsionRadius * 2.0f) {
            neighborCount++;
        }
    }
    
    return static_cast<float>(neighborCount);
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
    for (size_t i = 0; i < m_particles.size(); ++i) {
        const auto& particle = m_particles[i];
        
        // 为每个粒子创建模型矩阵
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, particle.position);
        
        // ✅ 调试可视化：中心粒子显示为稍大的球体
        if (m_debugVisualization && particle.isCore) {
            model = glm::scale(model, glm::vec3(1.5f));  // 中心粒子放大 1.5 倍
        } else {
            model = glm::scale(model, glm::vec3(1.0f));
        }
        
        m_shader->setMat4("uModel", model);
        
        // 绘制
        m_particleVAO->draw(GL_TRIANGLES, m_indexCount);
    }
    
    m_shader->end();
    
    // ✅ 调试可视化：绘制力作用范围（线框球体）
    // 注意：这需要额外的着色器和网格，暂时注释掉
    // if (m_debugVisualization && m_coreParticleIndex >= 0) {
    //     renderDebugSphere(m_particles[m_coreParticleIndex].position, m_forceRadius);
    // }
}

bool Slime::collideWith(const Object& other) const {
    // 碰撞由物理引擎处理
    return false;
}

void Slime::applyForce(const glm::vec3& force) {
    // ✅ 修复：大幅降低最大力度限制
    const float maxForcePerFrame = 20.0f;  // 降低单次最大力度（从 100 改为 20）
    glm::vec3 clampedForce = force;
    
    if (glm::length(force) > maxForcePerFrame) {
        clampedForce = glm::normalize(force) * maxForcePerFrame;
    }
    
    //区域性力传播
    if (m_coreParticleIndex < 0 || m_coreParticleIndex >= static_cast<int>(m_particles.size())) {
        // 如果没有有效的中心粒子，回退到旧行为（所有粒子受力）
        for (auto& particle : m_particles) {
            if (particle.rigidBody) {
                rp3d::Vector3 rp3dForce(clampedForce.x, clampedForce.y, clampedForce.z);
                particle.rigidBody->applyWorldForceAtCenterOfMass(rp3dForce);
            }
        }
        return;
    }
    
    // ✅ 区域性力传播：只对中心粒子附近的粒子施加力
    const glm::vec3& corePosition = m_particles[m_coreParticleIndex].position;
    
    for (size_t i = 0; i < m_particles.size(); ++i) {
        auto& particle = m_particles[i];
        if (!particle.rigidBody) continue;
        
        // 计算粒子到中心粒子的距离
        float distanceFromCore = glm::length(particle.position - corePosition);
        
        // 只对在力作用半径内的粒子施加力
        if (distanceFromCore > m_forceRadius) {
            continue;  // 跳过距离太远的粒子
        }

        // 使用平滑的衰减函数：(1 - distance/radius)^2
        float attenuation = 1.0f - (distanceFromCore / m_forceRadius);
        attenuation = attenuation * attenuation;  // 平方衰减，更平滑
        attenuation = glm::clamp(attenuation, 0.0f, 1.0f);
        
        // 应用衰减后的力
        glm::vec3 attenuatedForce = clampedForce * attenuation;
        rp3d::Vector3 rp3dForce(attenuatedForce.x, attenuatedForce.y, attenuatedForce.z);
        particle.rigidBody->applyWorldForceAtCenterOfMass(rp3dForce);
        
        // ✅ 大幅降低速度限制（防止失控）
        const float maxVelocity = 5.0f;  // 降低最大速度（从 20 改为 5）
        rp3d::Vector3 velocity = particle.rigidBody->getLinearVelocity();
        float speed = velocity.length();
        
        if (speed > maxVelocity) {
            velocity = (velocity / speed) * maxVelocity;
            particle.rigidBody->setLinearVelocity(velocity);
        }
    }
}

float Slime::getDistanceFromCore(int particleIndex) const {
    if (m_coreParticleIndex < 0 || particleIndex < 0 || 
        particleIndex >= static_cast<int>(m_particles.size())) {
        return std::numeric_limits<float>::max();
    }
    
    const glm::vec3& corePos = m_particles[m_coreParticleIndex].position;
    const glm::vec3& particlePos = m_particles[particleIndex].position;
    return glm::length(particlePos - corePos);
}
