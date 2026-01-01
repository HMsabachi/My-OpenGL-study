// slime.cpp
#include "slime.h"
#include "../../engine.h"
#include "../../wrapper/widgets.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <random>
#include <iostream>
#include <cmath>
#include <execution>  // ✅ C++17 并行算法
#include <numeric>    // ✅ std::iota
#include <thread>     // ✅ 线程支持
#include <mutex>      // ✅ 互斥锁
#include <atomic>     // ✅ 原子操作

// 常量
const float PI = 3.14159265359f;

Slime::Slime(Engine* engine, const glm::vec3& position, float radius, 
             int particleCount, Shader* shader, GLuint texture)
    : Object(engine, position),
      m_slimeRadius(radius),
      m_particleRadius(0.12f),
      m_restDensity(6000.0f),
      m_epsilon(600.0f),
      m_solverIterations(3),
      m_cohesionStrength(3.0f),
      m_viscosity(0.05f),
      m_shader(shader),
      m_texture(texture),
      m_sphereIndexCount(0)
{
    // 初始化粒子
    m_particles.resize(particleCount);
    m_neighbors.resize(particleCount);
    
    // ✅ 创建粒子索引数组（用于并行遍历）
    m_particleIndices.resize(particleCount);
    std::iota(m_particleIndices.begin(), m_particleIndices.end(), 0);
    
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
        
        offset *= radius * 0.9f;
        
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
    
    // ✅ 更新日志输出
    std::cout << "[Slime] 史莱姆创建成功：" << particleCount << " 个粒子 | 并行计算：启用" 
              << " | CPU 核心数：" << std::thread::hardware_concurrency() << std::endl;
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
    
    // ✅ PBF模拟步骤（纯并行优化）
    applyExternalForces(deltaTime);
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
    
    // ✅ 使用物理查询进行碰撞检测
    handlePhysicsCollisions();
    
    // 更新渲染数据
    updateInstanceBuffer();
    
    // ✅ 更新质心位置（用于相机跟踪，但不作为向心力中心）
    m_position = getCenterOfMass();
}

// ✅ 并行优化：施加外力（删除串行代码）
void Slime::applyExternalForces(float dt) {
    const glm::vec3 gravity(0.0f, -9.81f, 0.0f);
    
    // ✅ 纯并行处理
    std::for_each(std::execution::par_unseq, m_particles.begin(), m_particles.end(),
        [gravity](Particle& particle) {
            particle.force += gravity;
        });
}

// ✅ 并行优化：预测位置（删除串行代码）
void Slime::predictPositions(float dt) {
    // ✅ 使用 par_unseq 进一步优化，允许向量化
    std::for_each(std::execution::par_unseq, m_particles.begin(), m_particles.end(),
        [dt](Particle& particle) {
            particle.velocity += particle.force * dt;
            particle.predictedPos = particle.position + particle.velocity * dt;
            particle.force = glm::vec3(0.0f);
        });
}

// ✅ 并行优化：构建空间哈希（Lock-Free 优化）
void Slime::buildSpatialHash() {
    m_spatialHash.clear();
    
    // ✅ 第一步：并行计算所有粒子的哈希键
    std::vector<std::pair<int, int>> hashKeyPairs(m_particles.size());
    
    std::for_each(std::execution::par_unseq, m_particleIndices.begin(), m_particleIndices.end(),
        [this, &hashKeyPairs](int i) {
            int key = getHashKey(m_particles[i].predictedPos);
            hashKeyPairs[i] = {key, i};
        });
    
    // ✅ 第二步：串行合并到哈希表（这部分很快，不需要并行）
    for (const auto& [key, idx] : hashKeyPairs) {
        m_spatialHash[key].push_back(idx);
    }
}

// ✅ 并行优化：更新邻居（删除串行代码）
void Slime::updateNeighbors() {
    const float h = m_particleRadius * 4.0f;
    const float h_sq = h * h;  // ✅ 优化：避免重复计算平方根
    
    // ✅ 纯并行处理
    std::for_each(std::execution::par, m_particleIndices.begin(), m_particleIndices.end(),
        [this, h_sq](int i) {
            m_neighbors[i].clear();
            std::vector<int> candidates = getNeighbors(m_particles[i].predictedPos);
            
            // ✅ 预分配空间，减少动态分配
            m_neighbors[i].reserve(32);
            
            for (int j : candidates) {
                if (i == j) continue;
                
                // ✅ 优化：先用平方距离判断，避免 sqrt
                glm::vec3 diff = m_particles[i].predictedPos - m_particles[j].predictedPos;
                float dist_sq = glm::dot(diff, diff);
                
                if (dist_sq < h_sq) {
                    m_neighbors[i].push_back(j);
                }
            }
        });
}

// ✅ 并行优化：约束求解（删除串行代码）
void Slime::solveConstraints() {
    // ✅ 第一步：并行计算 lambda
    std::for_each(std::execution::par, m_particleIndices.begin(), m_particleIndices.end(),
        [this](int i) {
            m_particles[i].lambda = computeLambda(i);
        });
    
    // ✅ 第二步：并行计算位置修正
    std::for_each(std::execution::par, m_particleIndices.begin(), m_particleIndices.end(),
        [this](int i) {
            m_particles[i].deltaPos = computeDeltaP(i);
        });
    
    // ✅ 第三步：并行应用位置修正（使用 par_unseq 向量化）
    std::for_each(std::execution::par_unseq, m_particles.begin(), m_particles.end(),
        [](Particle& particle) {
            particle.predictedPos += particle.deltaPos;
        });
}

// ✅ 并行优化：更新速度（删除串行代码）
void Slime::updateVelocities(float dt) {
    const float invDt = 1.0f / dt;  // ✅ 优化：避免除法
    
    std::for_each(std::execution::par_unseq, m_particles.begin(), m_particles.end(),
        [invDt](Particle& particle) {
            particle.velocity = (particle.predictedPos - particle.position) * invDt;
            particle.position = particle.predictedPos;
        });
}

// ✅ 并行优化：向心力（删除串行代码，优化缓存局部性）
void Slime::applyCohesionForce() {
    const glm::vec3 centerOfMass = getCenterOfMass();
    const glm::vec3 targetCenter = centerOfMass + glm::vec3(0.0f, m_slimeRadius * 0.3f, 0.0f);
    
    // ✅ 预计算常量
    const float radiusThreshold = m_slimeRadius * 0.5f;
    const float invRadius = 1.0f / m_slimeRadius;
    const float idealDist = m_particleRadius * 2.2f;
    const float maxAttractionDist = m_particleRadius * 3.5f;
    const float attractionRange = maxAttractionDist - idealDist;
    const float maxForce = m_cohesionStrength * 3.0f;
    
    // ✅ 纯并行处理
    std::for_each(std::execution::par, m_particleIndices.begin(), m_particleIndices.end(),
        [this, centerOfMass, targetCenter, radiusThreshold, invRadius, 
         idealDist, maxAttractionDist, attractionRange, maxForce](int i) {
            auto& particle = m_particles[i];
            glm::vec3 toTarget = targetCenter - particle.position;
            float dist = glm::length(toTarget);
            
            if (dist < 0.001f) return;
            
            float distanceFromCenter = glm::length(particle.position - centerOfMass);
            float heightFactor = (particle.position.y - centerOfMass.y) * invRadius;
            heightFactor = glm::clamp(heightFactor, -1.0f, 1.0f);
            float verticalMultiplier = 1.0f + heightFactor * 1.5f;
            
            // 向心力
            if (distanceFromCenter > radiusThreshold) {
                float excessDist = distanceFromCenter - radiusThreshold;
                float forceMagnitude = m_cohesionStrength * (excessDist * invRadius) * verticalMultiplier;
                forceMagnitude = glm::min(forceMagnitude, maxForce);
                particle.force += glm::normalize(toTarget) * forceMagnitude;
            }
            
            // 表面张力（邻居吸引）
            for (int neighborIdx : m_neighbors[i]) {
                glm::vec3 toNeighbor = m_particles[neighborIdx].position - particle.position;
                float neighborDist = glm::length(toNeighbor);
                
                if (neighborDist > idealDist && neighborDist < maxAttractionDist) {
                    float attractionStrength = 0.8f * (1.0f - (neighborDist - idealDist) / attractionRange);
                    particle.force += glm::normalize(toNeighbor) * attractionStrength;
                }
            }
            
            // 向下挤压效果
            if (heightFactor < -0.2f) {
                glm::vec3 radialDir = particle.position - centerOfMass;
                radialDir.y = 0;
                float radialLen = glm::length(radialDir);
                
                if (radialLen > 0.001f) {
                    radialDir /= radialLen;  // normalize
                    float outwardForce = m_cohesionStrength * 0.5f * (-heightFactor - 0.2f);
                    particle.force += radialDir * outwardForce;
                }
            }
        });
}

// ✅ 并行优化：粘性（删除串行代码）
void Slime::applyViscosity() {
    std::for_each(std::execution::par, m_particleIndices.begin(), m_particleIndices.end(),
        [this](int i) {
            glm::vec3 velocityChange(0.0f);
            int neighborCount = m_neighbors[i].size();
            
            if (neighborCount == 0) return;
            
            // ✅ 优化：累加邻居速度差
            for (int neighborIdx : m_neighbors[i]) {
                velocityChange += m_particles[neighborIdx].velocity - m_particles[i].velocity;
            }
            
            // ✅ 优化：一次除法
            velocityChange /= static_cast<float>(neighborCount);
            m_particles[i].velocity += m_viscosity * velocityChange;
        });
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
    
    // 加入自身质量的贡献
    density += poly6Kernel(0.0f, h);
    
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
    
    if (std::abs(C) < 0.0001f) {
        return 0.0f;
    }
    
    glm::vec3 gradientSum(0.0f);
    float gradientSumSq = 0.0f;
    
    for (int neighborIdx : m_neighbors[particleIdx]) {
        glm::vec3 diff = m_particles[particleIdx].predictedPos - m_particles[neighborIdx].predictedPos;
        glm::vec3 gradient = spikyGradient(diff, h) / m_restDensity;
        gradientSum += gradient;
        gradientSumSq += glm::dot(gradient, gradient);
    }
    
    gradientSumSq += glm::dot(gradientSum, gradientSum);
    
    return -C / (gradientSumSq + m_epsilon);
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

int Slime::getHashKey(const glm::vec3& pos) {
    int x = static_cast<int>(std::floor(pos.x / m_cellSize));
    int y = static_cast<int>(std::floor(pos.y / m_cellSize));
    int z = static_cast<int>(std::floor(pos.z / m_cellSize));
    
    // 简单哈希函数
    return (x * 73856093) ^ (y * 19349663) ^ (z * 83492791);
}

std::vector<int> Slime::getNeighbors(const glm::vec3& pos) {
    std::vector<int> neighbors;
    neighbors.reserve(64);  // ✅ 预分配空间
    
    // 检查当前格子和周围26个格子
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dz = -1; dz <= 1; ++dz) {
                glm::vec3 offset = glm::vec3(dx, dy, dz) * m_cellSize;
                int key = getHashKey(pos + offset);
                
                auto it = m_spatialHash.find(key);
                if (it != m_spatialHash.end()) {
                    neighbors.insert(neighbors.end(), it->second.begin(), it->second.end());
                }
            }
        }
    }
    
    return neighbors;
}

// ✅ 优化：并行碰撞检测（分块处理）
void Slime::handlePhysicsCollisions() {
    if (!m_engine) return;
    
    auto* world = m_engine->getPhysicsWorld();
    if (!world) return;
    
    const float checkDistance = m_particleRadius * 2.0f;
    const float restitution = 0.3f;
    const float friction = 0.4f;
    const float minSpeed = 0.01f;  // 最小速度阈值
    
    // ✅ 并行处理碰撞检测
    std::for_each(std::execution::par, m_particleIndices.begin(), m_particleIndices.end(),
        [this, world, checkDistance, restitution, friction, minSpeed](int idx) {
            auto& particle = m_particles[idx];
            float speed = glm::length(particle.velocity);
            
            if (speed < minSpeed) return;  // 静止粒子跳过
            
            // 向速度方向发射射线
            glm::vec3 rayDir = glm::normalize(particle.velocity);
            float rayLength = speed * 0.016f + checkDistance;
            
            rp3d::Vector3 start(particle.position.x, particle.position.y, particle.position.z);
            rp3d::Vector3 end = start + rp3d::Vector3(rayDir.x, rayDir.y, rayDir.z) * rayLength;
            rp3d::Ray ray(start, end);
            
            // Raycast 回调
            class SlimeRaycastCallback : public rp3d::RaycastCallback {
            public:
                bool hasHit = false;
                glm::vec3 hitNormal;
                glm::vec3 hitPoint;
                float hitFraction = 1.0f;
                
                virtual rp3d::decimal notifyRaycastHit(const rp3d::RaycastInfo& info) override {
                    if (info.hitFraction < hitFraction) {
                        hasHit = true;
                        hitNormal = glm::vec3(info.worldNormal.x, info.worldNormal.y, info.worldNormal.z);
                        hitPoint = glm::vec3(info.worldPoint.x, info.worldPoint.y, info.worldPoint.z);
                        hitFraction = info.hitFraction;
                    }
                    return info.hitFraction;
                }
            };
            
            SlimeRaycastCallback callback;
            world->raycast(ray, &callback);
            
            if (callback.hasHit) {
                float penetration = m_particleRadius - glm::length(callback.hitPoint - particle.position);
                
                if (penetration > 0) {
                    // 位置修正
                    particle.position += callback.hitNormal * penetration;
                    particle.predictedPos = particle.position;
                    
                    // 速度修正
                    float vn = glm::dot(particle.velocity, callback.hitNormal);
                    if (vn < 0) {
                        glm::vec3 normalVel = vn * callback.hitNormal;
                        particle.velocity -= (1.0f + restitution) * normalVel;
                        
                        glm::vec3 tangentVel = particle.velocity - glm::dot(particle.velocity, callback.hitNormal) * callback.hitNormal;
                        particle.velocity -= tangentVel * friction;
                    }
                }
            }
        });
}

// ✅ 优化：并行更新实例缓冲
void Slime::updateInstanceBuffer() {
    std::vector<float> instanceData(m_particles.size() * 16);
    
    // ✅ 并行生成矩阵数据
    std::for_each(std::execution::par_unseq, m_particleIndices.begin(), m_particleIndices.end(),
        [this, &instanceData](int i) {
            glm::mat4 matrix = glm::translate(glm::mat4(1.0f), m_particles[i].position);
            memcpy(&instanceData[i * 16], glm::value_ptr(matrix), 16 * sizeof(float));
        });
    
    // ✅ 使用封装的 update 方法更新GPU缓冲
    m_instanceVBO->update(instanceData, 0);
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
    const float forcePerParticle = 1.0f / static_cast<float>(m_particles.size());
    const glm::vec3 distributedForce = force * forcePerParticle;
    
    // ✅ 并行施加力
    std::for_each(std::execution::par_unseq, m_particles.begin(), m_particles.end(),
        [distributedForce](Particle& particle) {
            particle.force += distributedForce;
        });
}

glm::vec3 Slime::getCenterOfMass() const {
    // ✅ 使用并行循环累加位置，然后求平均
    std::vector<glm::vec3> positions(m_particles.size());
    
    // 提取所有粒子位置
    std::transform(std::execution::par_unseq,
                   m_particles.begin(), m_particles.end(),
                   positions.begin(),
                   [](const Particle& p) { return p.position; });
    
    // 并行求和所有位置
    glm::vec3 center = std::reduce(std::execution::par_unseq, 
                                    positions.begin(), 
                                    positions.end(),
                                    glm::vec3(0.0f),
                                    [](const glm::vec3& a, const glm::vec3& b) {
                                        return a + b;
                                    });
    
    return center / static_cast<float>(m_particles.size());
}
