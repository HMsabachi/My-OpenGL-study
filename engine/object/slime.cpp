// slime.cpp
#include "slime.h"
#include "../engine.h"
#include "../../wrapper/widgets.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <random>
#include <iostream>
#include <cmath>

// 常量
const float PI = 3.14159265359f;

Slime::Slime(Engine* engine, const glm::vec3& position, float radius, 
             int particleCount, Shader* shader, GLuint texture)
    : Object(engine, position),
      m_slimeRadius(radius),
      m_particleRadius(0.12f),        // 增大粒子半径
      m_restDensity(1000.0f),         // 大幅降低静止密度（原来6378）
      m_epsilon(100.0f),              // 降低松弛参数（原来600）
      m_solverIterations(3),          // 减少迭代次数
      m_cohesionStrength(3.0f),       // 大幅降低向心力（原来15.0）
      m_viscosity(0.05f),             // 增加粘性
      m_shader(shader),
      m_texture(texture),
      m_sphereIndexCount(0)
{
    // 初始化粒子
    m_particles.resize(particleCount);
    m_neighbors.resize(particleCount);
    
    // 在球体内随机分布粒子
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    for (auto& particle : m_particles) {
        // 在球体内均匀分布
        glm::vec3 offset;
        do {
            offset = glm::vec3(dist(gen), dist(gen), dist(gen));
        } while (glm::length(offset) > 1.0f);
        
        offset *= radius * 0.9f;  // 更松散的初始分布
        
        particle.position = position + offset;
        particle.predictedPos = particle.position;
        particle.velocity = glm::vec3(0.0f);
        particle.force = glm::vec3(0.0f);
        particle.lambda = 0.0f;
        particle.deltaPos = glm::vec3(0.0f);
    }
    
    // 设置网格大小为粒子搜索半径
    m_cellSize = m_particleRadius * 4.0f;
    
    // 初始化渲染数据
    initRenderData();
    
    std::cout << "[Slime] 史莱姆创建成功：" << particleCount << " 个粒子" << std::endl;
}

Slime::~Slime() {
    delete m_vao;
}

void Slime::initRenderData() {
    // 创建一个小球体网格作为粒子的基础模型
    widgets::SphereData sphereData = widgets::createSphere(m_particleRadius, 8, 6);
    
    m_sphereVBO = std::make_shared<Buffer<float>>(sphereData.vertices, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    m_sphereEBO = std::make_shared<Buffer<unsigned int>>(sphereData.indices, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
    m_sphereIndexCount = sphereData.indices.size();
    
    // 创建实例化矩阵缓冲（作为float数组）
    std::vector<float> instanceData(m_particles.size() * 16);  // mat4 = 16个float
    for (size_t i = 0; i < m_particles.size(); ++i) {
        glm::mat4 matrix = glm::translate(glm::mat4(1.0f), m_particles[i].position);
        // 将矩阵数据复制到float数组
        memcpy(&instanceData[i * 16], glm::value_ptr(matrix), 16 * sizeof(float));
    }
    
    m_instanceVBO = std::make_shared<Buffer<float>>(instanceData, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
    
    // 配置VAO
    m_vao = new VAO();
    m_vao->addVBO(*m_sphereVBO, "3f 3f 2f", GL_FALSE, 0);  // 顶点数据: pos, normal, texCoord
    
    // 添加实例化数据（mat4需要占用4个属性位置）
    m_vao->addInstancedVBO(*m_instanceVBO, "4f 4f 4f 4f", 3, 1);  // 从location 3开始
    
    m_vao->addEBO(*m_sphereEBO);
}

void Slime::update(float deltaTime) {
    // 限制时间步长，避免不稳定
    const float maxDt = 0.016f;  // 约60fps
    deltaTime = std::min(deltaTime, maxDt);
    
    // PBF模拟步骤
    applyExternalForces(deltaTime);
    applyCohesionForce();  // 在这里施加向心力（作为外力）
    predictPositions(deltaTime);
    
    // 构建空间哈希和更新邻居
    buildSpatialHash();
    updateNeighbors();
    
    // 迭代求解约束
    for (int i = 0; i < m_solverIterations; ++i) {
        solveConstraints();
    }
    
    updateVelocities(deltaTime);
    applyViscosity();
    
    // 处理碰撞
    handleCollisions();
    handleObjectCollisions();
    
    // 安全检查：确保粒子不会离质心太远
    glm::vec3 center = getCenterOfMass();
    const float maxDistance = m_slimeRadius * 3.0f;
    for (auto& particle : m_particles) {
        float dist = glm::length(particle.position - center);
        if (dist > maxDistance) {
            // 强制拉回
            particle.position = center + glm::normalize(particle.position - center) * maxDistance;
            particle.velocity *= 0.5f;
        }
    }
    
    // 更新渲染数据
    updateInstanceBuffer();
    
    // 更新质心位置
    m_position = center;
}

void Slime::applyExternalForces(float dt) {
    // 应用重力和外部力
    glm::vec3 gravity(0.0f, -9.81f, 0.0f);
    
    for (auto& particle : m_particles) {
        particle.force = gravity + particle.force;
    }
}

void Slime::predictPositions(float dt) {
    for (auto& particle : m_particles) {
        particle.velocity += particle.force * dt;
        particle.predictedPos = particle.position + particle.velocity * dt;
        particle.force = glm::vec3(0.0f);  // 重置力
    }
}

void Slime::buildSpatialHash() {
    m_spatialHash.clear();
    
    for (int i = 0; i < m_particles.size(); ++i) {
        int key = getHashKey(m_particles[i].predictedPos);
        m_spatialHash[key].push_back(i);
    }
}

int Slime::getHashKey(const glm::vec3& pos) {
    int x = static_cast<int>(std::floor(pos.x / m_cellSize));
    int y = static_cast<int>(std::floor(pos.y / m_cellSize));
    int z = static_cast<int>(std::floor(pos.z / m_cellSize));
    
    // 简单哈希函数
    return (x * 73856093) ^ (y * 19349663) ^ (z * 83492791);
}

std::vector<int> Slime::getNeighbors(const glm::vec3& pos) {
    std::vector<int> neighbors;
    
    // 检查当前格子和周围26个格子
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dz = -1; dz <= 1; ++dz) {
                glm::vec3 offset = glm::vec3(dx, dy, dz) * m_cellSize;
                int key = getHashKey(pos + offset);
                
                if (m_spatialHash.find(key) != m_spatialHash.end()) {
                    for (int idx : m_spatialHash[key]) {
                        neighbors.push_back(idx);
                    }
                }
            }
        }
    }
    
    return neighbors;
}

void Slime::updateNeighbors() {
    float h = m_particleRadius * 4.0f;  // 搜索半径
    
    for (int i = 0; i < m_particles.size(); ++i) {
        m_neighbors[i].clear();
        std::vector<int> candidates = getNeighbors(m_particles[i].predictedPos);
        
        for (int j : candidates) {
            if (i == j) continue;
            float dist = glm::length(m_particles[i].predictedPos - m_particles[j].predictedPos);
            if (dist < h) {
                m_neighbors[i].push_back(j);
            }
        }
    }
}

float Slime::poly6Kernel(float r, float h) {
    if (r >= h) return 0.0f;
    float scale = 315.0f / (64.0f * PI * std::pow(h, 9.0f));
    float x = h * h - r * r;
    return scale * x * x * x;
}

glm::vec3 Slime::spikyGradient(const glm::vec3& r, float h) {
    float rLen = glm::length(r);
    if (rLen >= h || rLen < 0.0001f) return glm::vec3(0.0f);
    
    float scale = -45.0f / (PI * std::pow(h, 6.0f));
    float x = h - rLen;
    return scale * x * x * (r / rLen);
}

float Slime::computeDensity(int particleIdx) {
    float density = 0.0f;
    float h = m_particleRadius * 4.0f;
    
    for (int neighborIdx : m_neighbors[particleIdx]) {
        glm::vec3 diff = m_particles[particleIdx].predictedPos - m_particles[neighborIdx].predictedPos;
        float r = glm::length(diff);
        density += poly6Kernel(r, h);
    }
    
    return density;
}

float Slime::computeLambda(int particleIdx) {
    float h = m_particleRadius * 4.0f;
    float density = computeDensity(particleIdx);
    float C = density / m_restDensity - 1.0f;
    
    // 只在密度大于静止密度时才进行约束
    if (C > 0.0f) {
        glm::vec3 gradientSum(0.0f);
        float gradientSumSq = 0.0f;
        
        for (int neighborIdx : m_neighbors[particleIdx]) {
            glm::vec3 diff = m_particles[particleIdx].predictedPos - m_particles[neighborIdx].predictedPos;
            glm::vec3 gradient = spikyGradient(diff, h) / m_restDensity;
            gradientSum += gradient;
            gradientSumSq += glm::dot(gradient, gradient);
        }
        
        gradientSumSq += glm::dot(gradientSum, gradientSum);
        
        // 添加更强的松弛以避免过度约束
        return -C / (gradientSumSq + m_epsilon);
    }
    
    return 0.0f;
}

glm::vec3 Slime::computeDeltaP(int particleIdx) {
    glm::vec3 deltaP(0.0f);
    float h = m_particleRadius * 4.0f;
    float lambda_i = m_particles[particleIdx].lambda;
    
    for (int neighborIdx : m_neighbors[particleIdx]) {
        float lambda_j = m_particles[neighborIdx].lambda;
        glm::vec3 diff = m_particles[particleIdx].predictedPos - m_particles[neighborIdx].predictedPos;
        glm::vec3 gradient = spikyGradient(diff, h);
        
        deltaP += (lambda_i + lambda_j) * gradient;
    }
    
    return deltaP / m_restDensity;
}

void Slime::solveConstraints() {
    // 计算lambda
    for (int i = 0; i < m_particles.size(); ++i) {
        m_particles[i].lambda = computeLambda(i);
    }
    
    // 计算位置修正
    for (int i = 0; i < m_particles.size(); ++i) {
        m_particles[i].deltaPos = computeDeltaP(i);
    }
    
    // 应用位置修正
    for (auto& particle : m_particles) {
        particle.predictedPos += particle.deltaPos;
    }
}

void Slime::updateVelocities(float dt) {
    const float maxVelocity = 50.0f;  // 限制最大速度
    
    for (auto& particle : m_particles) {
        particle.velocity = (particle.predictedPos - particle.position) / dt;
        
        // 限制速度，防止粒子飞出
        float speed = glm::length(particle.velocity);
        if (speed > maxVelocity) {
            particle.velocity = (particle.velocity / speed) * maxVelocity;
        }
        
        particle.position = particle.predictedPos;
    }
}

void Slime::applyCohesionForce() {
    // 向心力：形成史莱姆的水滴形状（底部扁平宽，顶部圆润）
    glm::vec3 centerOfMass = getCenterOfMass();
    
    for (size_t i = 0; i < m_particles.size(); ++i) {
        auto& particle = m_particles[i];
        
        // 计算粒子相对质心的位置
        glm::vec3 relativePos = particle.position - centerOfMass;
        float horizontalDist = std::sqrt(relativePos.x * relativePos.x + relativePos.z * relativePos.z);
        float verticalPos = relativePos.y;
        
        // 定义史莱姆的理想形状（水滴形）
        // 计算此高度下的理想水平半径
        float heightRatio = (verticalPos + m_slimeRadius * 0.5f) / (m_slimeRadius * 1.5f);
        heightRatio = glm::clamp(heightRatio, 0.0f, 1.0f);
        
        // 水滴形状：底部宽，顶部窄
        float idealRadius;
        if (heightRatio < 0.5f) {
            // 底部保持宽度
            idealRadius = m_slimeRadius * 1.2f;
        } else {
            // 顶部收窄
            float t = (heightRatio - 0.5f) / 0.5f;
            idealRadius = m_slimeRadius * (1.2f - 0.8f * t * t);
        }
        
        // 1. 水平向心力：约束水平半径
        if (horizontalDist > idealRadius * 0.1f) {  // 避免除以0
            float excess = horizontalDist - idealRadius;
            if (excess > 0) {
                glm::vec3 horizontalDir = glm::normalize(glm::vec3(relativePos.x, 0, relativePos.z));
                float strength = m_cohesionStrength * excess;
                particle.force -= horizontalDir * strength;
            }
        }
        
        // 2. 垂直力：维持高度
        // 底部支撑
        if (verticalPos < -m_slimeRadius * 0.4f) {
            float depth = -m_slimeRadius * 0.4f - verticalPos;
            particle.force.y += m_cohesionStrength * depth * 3.0f;
        }
        // 顶部压制（防止过高）
        else if (verticalPos > m_slimeRadius * 0.6f) {
            float excess = verticalPos - m_slimeRadius * 0.6f;
            particle.force.y -= m_cohesionStrength * excess * 2.0f;
        }
        
        // 3. 表面张力
        for (int neighborIdx : m_neighbors[i]) {
            glm::vec3 toNeighbor = m_particles[neighborIdx].position - particle.position;
            float neighborDist = glm::length(toNeighbor);
            
            if (neighborDist > m_particleRadius * 2.0f && neighborDist < m_particleRadius * 3.5f) {
                float attractionStrength = 0.5f;
                particle.force += glm::normalize(toNeighbor) * attractionStrength;
            }
        }
    }
}

void Slime::applyViscosity() {
    // XSPH粘性
    for (int i = 0; i < m_particles.size(); ++i) {
        glm::vec3 velocityChange(0.0f);
        
        for (int neighborIdx : m_neighbors[i]) {
            glm::vec3 velocityDiff = m_particles[neighborIdx].velocity - m_particles[i].velocity;
            velocityChange += velocityDiff;
        }
        
        if (m_neighbors[i].size() > 0) {
            velocityChange /= static_cast<float>(m_neighbors[i].size());
            m_particles[i].velocity += m_viscosity * velocityChange;
        }
    }
}

void Slime::handleCollisions() {
    // 简单地面碰撞
    const float groundY = -5.0f;
    const float damping = 0.3f;  // 增加阻尼，让碰撞更明显
    
    for (auto& particle : m_particles) {
        if (particle.position.y - m_particleRadius < groundY) {
            particle.position.y = groundY + m_particleRadius;
            particle.velocity.y *= -damping;
            
            // 添加摩擦力
            particle.velocity.x *= 0.95f;
            particle.velocity.z *= 0.95f;
        }
    }
}

void Slime::handleObjectCollisions() {
    // 与ReactPhysics3D对象的简单碰撞检测
    if (!m_engine) return;
    
    auto* world = m_engine->getPhysicsWorld();
    if (!world) return;
    
    // 创建一个Raycast回调类
    class CollisionCallback : public rp3d::RaycastCallback {
    public:
        bool hasHit = false;
        glm::vec3 hitNormal;
        
        virtual rp3d::decimal notifyRaycastHit(const rp3d::RaycastInfo& raycastInfo) override {
            hasHit = true;
            hitNormal = glm::vec3(
                raycastInfo.worldNormal.x,
                raycastInfo.worldNormal.y,
                raycastInfo.worldNormal.z
            );
            return raycastInfo.hitFraction;  // 返回命中分数，继续搜索更近的
        }
    };
    
    // 遍历所有粒子，检测与物理世界的碰撞
    for (auto& particle : m_particles) {
        rp3d::Vector3 start(particle.position.x, particle.position.y, particle.position.z);
        rp3d::Vector3 end = start + rp3d::Vector3(0, -m_particleRadius * 2, 0);
        
        rp3d::Ray ray(start, end);
        CollisionCallback callback;
        
        world->raycast(ray, &callback);
        
        if (callback.hasHit) {
            // 发生碰撞，调整位置
            glm::vec3 normal = callback.hitNormal;
            
            // 将粒子推出碰撞体
            particle.position += normal * m_particleRadius * 0.5f;
            
            // 反弹
            float restitution = 0.3f;
            particle.velocity -= (1.0f + restitution) * glm::dot(particle.velocity, normal) * normal;
        }
    }
}

void Slime::updateInstanceBuffer() {
    // 更新所有粒子的变换矩阵
    std::vector<float> instanceData(m_particles.size() * 16);  // mat4 = 16个float
    
    for (size_t i = 0; i < m_particles.size(); ++i) {
        glm::mat4 matrix = glm::translate(glm::mat4(1.0f), m_particles[i].position);
        // 将矩阵数据复制到float数组
        memcpy(&instanceData[i * 16], glm::value_ptr(matrix), 16 * sizeof(float));
    }
    
    // 更新GPU缓冲
    m_instanceVBO->bind();
    glBufferSubData(GL_ARRAY_BUFFER, 0, 
                    instanceData.size() * sizeof(float), 
                    instanceData.data());
    m_instanceVBO->unbind();
}

void Slime::render() const {
    if (!m_shader) return;
    
    m_shader->begin();
    
    // 绑定纹理（如果有）
    if (m_texture > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);
    }
    
    // 设置史莱姆颜色
    m_shader->set("uSlimeColor", glm::vec3(0.3f, 1.0f, 0.5f));
    
    // 实例化绘制所有粒子
    m_vao->drawInstanced(m_particles.size(), m_sphereIndexCount);
    
    m_shader->end();
}

bool Slime::collideWith(const Object& other) const {
    // PBF粒子系统使用自己的碰撞检测
    return false;
}

void Slime::applyForce(const glm::vec3& force) {
    // 将力施加到所有粒子上
    for (auto& particle : m_particles) {
        particle.force += force / static_cast<float>(m_particles.size());
    }
}

glm::vec3 Slime::getCenterOfMass() const {
    glm::vec3 center(0.0f);
    for (const auto& particle : m_particles) {
        center += particle.position;
    }
    return center / static_cast<float>(m_particles.size());
}
