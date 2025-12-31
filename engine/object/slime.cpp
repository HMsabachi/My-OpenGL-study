#include "slime.h"
#include "../engine.h"
#include "../../wrapper/widgets.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

// 常量定义
constexpr float PI = 3.14159265359f;
constexpr float EPSILON = 1e-6f;

Slime::Slime(Engine* engine, const glm::vec3& center, float radius, int particleCount,
             Shader* shader, GLuint texture)
    : Object(engine, center), 
      m_shader(shader),
      m_texture(texture),
      m_particleRadius(0.12f),
      m_boundaryRadius(radius),
      m_slimeCenter(center),
      m_indexCount(0),
      m_externalForce(0.0f)
{
    setPosition(center);
    
    // ✅ 修正：使用更稳定的 PBF 参数 (基于 Macklin & Müller 2013)
    m_pbfParams.restDensity = 6378.0f;      // 调整为适合粒子间距的密度
    m_pbfParams.epsilon = 600.0f;           // CFM 松弛参数
    m_pbfParams.viscosity = 0.01f;          // XSPH 粘度
    m_pbfParams.surfaceTension = 0.0001f;   // 表面张力
    m_pbfParams.cohesion = 0.005f;          // 向心力（保持形状）
    m_pbfParams.smoothingRadius = 0.1f;     // ✅ 核半径 (根据粒子密度调整)
    m_pbfParams.solverIterations = 4;       // 求解器迭代次数
    m_pbfParams.vorticityEpsilon = 0.001f;
    m_pbfParams.boundaryDamping = 0.8f;
    m_pbfParams.deltaQ = 0.3f * m_pbfParams.smoothingRadius;
    m_pbfParams.tensileK = 0.0001f;
    m_pbfParams.tensileN = 4;
    
    m_cellSize = m_pbfParams.smoothingRadius;
    
    // ✅ 计算粒子质量 (基于密度和粒子体积)
    float particleVolume = (4.0f / 3.0f) * PI * std::pow(m_particleRadius, 3.0f);
    m_particleMass = m_pbfParams.restDensity * particleVolume / static_cast<float>(particleCount);
    
    // 初始化粒子
    initParticles(center, radius, particleCount);
    
    // ✅ 预计算核函数常量
    precomputeKernelConstants();
    
    std::cout << "✅ 史莱姆创建完成:" << std::endl;
    std::cout << "   位置: (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;
    std::cout << "   粒子数量: " << m_particles.size() << std::endl;
    std::cout << "   粒子质量: " << m_particleMass << " kg" << std::endl;
    std::cout << "   粒子半径: " << m_particleRadius << " m" << std::endl;
    std::cout << "   核半径: " << m_pbfParams.smoothingRadius << " m" << std::endl;
    std::cout << "   静止密度: " << m_pbfParams.restDensity << " kg/m³" << std::endl;
    
    // 初始化渲染网格
    initMesh();
    
    // ✅ 修正：初始化物理刚体 (代表史莱姆整体)
    // 使用球体碰撞体，质量为所有粒子质量之和
    float totalMass = m_particleMass * static_cast<float>(particleCount);
    initPhysics(PhysicsType::KINEMATIC, CollisionShape::SPHERE, 
                glm::vec3(radius), totalMass);
    
    // ✅ 设置刚体为运动学（kinematic），防止重力影响整体
    if (m_rigidBody) {
        m_rigidBody->setType(rp3d::BodyType::KINEMATIC);
        m_rigidBody->enableGravity(false);  // 禁用重力，粒子自己处理
    }
}

Slime::~Slime() {
    delete m_vao;
}

void Slime::precomputeKernelConstants() {
    float h = m_pbfParams.smoothingRadius;
    float h2 = h * h;
    float h3 = h2 * h;
    float h6 = h3 * h3;
    float h9 = h6 * h3;
    
    // Poly6 核常量
    m_poly6Constant = 315.0f / (64.0f * PI * h9);
    
    // Spiky 梯度核常量
    m_spikyGradConstant = -45.0f / (PI * h6);
    
    // 粘度拉普拉斯核常量
    m_viscosityLapConstant = 45.0f / (PI * h6);
    
    // 预计算 W(deltaQ)^n
    float deltaQ = m_pbfParams.deltaQ;
    float wDeltaQ = poly6Kernel(deltaQ);
    m_wDeltaQPow = std::pow(wDeltaQ, static_cast<float>(m_pbfParams.tensileN));
}

void Slime::initParticles(const glm::vec3& center, float radius, int count) {
    m_particles.clear();
    m_particles.reserve(count);
    m_neighbors.resize(count);
    
    // ✅ 改进：使用更均匀的初始分布
    // 方法：泊松圆盘采样 (简化版：随机 + 最小距离检查)
    float minDistance = m_pbfParams.smoothingRadius * 0.5f;
    
    for (int i = 0; i < count; ++i) {
        Particle p;
        bool validPosition = false;
        int attempts = 0;
        
        while (!validPosition && attempts < 30) {
            // 球坐标随机生成
            float r = std::pow(glm::linearRand(0.0f, 1.0f), 1.0f/3.0f) * radius * 0.9f;
            float theta = glm::linearRand(0.0f, 2.0f * PI);
            float phi = std::acos(glm::linearRand(-1.0f, 1.0f));
            
            glm::vec3 testPos = center + glm::vec3(
                r * std::sin(phi) * std::cos(theta),
                r * std::sin(phi) * std::sin(theta),
                r * std::cos(phi)
            );
            
            // 检查与已有粒子的最小距离
            validPosition = true;
            for (size_t j = 0; j < m_particles.size(); ++j) {
                if (glm::length(testPos - m_particles[j].position) < minDistance) {
                    validPosition = false;
                    break;
                }
            }
            
            if (validPosition) {
                p.position = testPos;
            }
            
            attempts++;
        }
        
        // 如果多次尝试仍失败，使用随机位置
        if (!validPosition) {
            float r = std::pow(glm::linearRand(0.0f, 1.0f), 1.0f/3.0f) * radius * 0.9f;
            float theta = glm::linearRand(0.0f, 2.0f * PI);
            float phi = std::acos(glm::linearRand(-1.0f, 1.0f));
            
            p.position = center + glm::vec3(
                r * std::sin(phi) * std::cos(theta),
                r * std::sin(phi) * std::sin(theta),
                r * std::cos(phi)
            );
        }
        
        p.velocity = glm::vec3(0.0f);
        p.mass = m_particleMass;
        p.density = 0.0f;
        p.lambda = 0.0f;
        p.deltaP = glm::vec3(0.0f);
        
        m_particles.push_back(p);
    }
    
    std::cout << "   ✅ 粒子初始化完成，平均间距约: " 
              << (2.0f * radius / std::cbrt(static_cast<float>(count))) << " m" << std::endl;
}

void Slime::initMesh() {
    // 创建球体网格
    auto sphereData = widgets::createSphere(m_particleRadius, 16, 16);
    m_indexCount = sphereData.indices.size();
    
    m_vbo = std::make_shared<Buffer<float>>(sphereData.vertices, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    m_ebo = std::make_shared<Buffer<unsigned int>>(sphereData.indices, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
    
    // 创建实例化矩阵缓冲
    std::vector<float> instanceData(m_particles.size() * 16);
    for (size_t i = 0; i < m_particles.size(); ++i) {
        glm::mat4 mat(1.0f);
        const float* matPtr = glm::value_ptr(mat);
        std::memcpy(&instanceData[i * 16], matPtr, 16 * sizeof(float));
    }
    m_instanceVBO = std::make_shared<Buffer<float>>(instanceData, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
    
    // 设置 VAO
    m_vao = new VAO();
    m_vao->addVBO(*m_vbo, "3f 3f 2f", GL_FALSE, 0);
    m_vao->addEBO(*m_ebo);
    m_vao->addInstancedVBO(*m_instanceVBO, "4f 4f 4f 4f", 3, 1);
}

void Slime::update(float deltaTime) {
    // ✅ 限制时间步长 - 更严格，避免不稳定
    float dt = glm::clamp(deltaTime, 0.001f, 0.016f);  // 最大 60 FPS 步长
    
    // ✅ 子步长模拟 (提高稳定性)
    const int subSteps = 2;
    float subDt = dt / static_cast<float>(subSteps);
    
    for (int i = 0; i < subSteps; ++i) {
        updatePBF(subDt);
    }
    
    // 更新质心
    m_slimeCenter = getSlimeCenter();
    setPosition(m_slimeCenter);
    
    // ✅ 同步到物理引擎 (更新刚体位置)
    syncToPhysics();
    
    // ✅ 与物理世界交互 (检测碰撞)
    interactWithPhysicsWorld();
    
    // 更新渲染数据
    updateInstancedData();
}

void Slime::updatePBF(float dt) {
    // ===== PBF 算法流程 (Macklin & Müller 2013) =====
    
    // 1. 应用外力
    applyExternalForces(dt);
    
    // 2. 预测位置
    predictPositions(dt);
    
    // 3. 查找邻居
    findNeighbors();
    
    // 4. 求解器循环
    resetDeltaP();
    for (int iter = 0; iter < m_pbfParams.solverIterations; ++iter) {
        // 4a. 计算密度和 lambda
        computeDensityAndLambda();
        
        // 4b. 计算位置修正
        computePositionCorrection();
        
        // 4c. 应用向心力（史莱姆特性）
        if (iter == m_pbfParams.solverIterations - 1) {
            applyCohesionForce();
        }
    }
    
    // 5. 应用位置修正
    applyPositionCorrection();
    
    // 6. 碰撞检测
    handleCollisions();
    
    // 7. 更新速度
    updateVelocities(dt);
    
    // 8. XSPH 粘度
    applyXSPHViscosity();
    
    // 9. 涡量约束 (可选)
    // applyVorticityConfinement(dt);
}

void Slime::applyExternalForces(float dt) {
    glm::vec3 gravity(0.0f, -9.81f, 0.0f);
    
    for (auto& p : m_particles) {
        // 重力 + 外部力 (来自玩家输入或物理引擎)
        p.velocity += (gravity + m_externalForce / m_particleMass) * dt;
    }
    
    // 清空外部力累积
    m_externalForce = glm::vec3(0.0f);
}

void Slime::predictPositions(float dt) {
    for (auto& p : m_particles) {
        p.predictedPosition = p.position + p.velocity * dt;
    }
}

void Slime::findNeighbors() {
    clearSpatialHash();
    
    // 插入所有粒子
    for (size_t i = 0; i < m_particles.size(); ++i) {
        insertParticle(i);
    }
    
    // 查找邻居
    float h = m_pbfParams.smoothingRadius;
    
    for (size_t i = 0; i < m_particles.size(); ++i) {
        m_neighbors[i].clear();
        
        const glm::vec3& pi = m_particles[i].predictedPosition;
        
        // 检查周围 27 个格子
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    glm::vec3 offset(dx * m_cellSize, dy * m_cellSize, dz * m_cellSize);
                    int neighborHash = hashPosition(pi + offset);
                    
                    auto it = m_spatialHash.find(neighborHash);
                    if (it != m_spatialHash.end()) {
                        for (int j : it->second) {
                            if (i != static_cast<size_t>(j)) {
                                float dist = glm::length(pi - m_particles[j].predictedPosition);
                                if (dist < h) {
                                    m_neighbors[i].push_back(j);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void Slime::resetDeltaP() {
    for (auto& p : m_particles) {
        p.deltaP = glm::vec3(0.0f);
    }
}

void Slime::computeDensityAndLambda() {
    float h = m_pbfParams.smoothingRadius;
    float rho0 = m_pbfParams.restDensity;
    
    for (size_t i = 0; i < m_particles.size(); ++i) {
        auto& pi = m_particles[i];
        const glm::vec3& xi = pi.predictedPosition;
        
        // 计算密度 ρ_i = Σ m_j * W(x_i - x_j, h)
        pi.density = pi.mass * poly6Kernel(0.0f);  // 自身贡献
        
        for (int j : m_neighbors[i]) {
            float r = glm::length(xi - m_particles[j].predictedPosition);
            pi.density += m_particles[j].mass * poly6Kernel(r);
        }
        
        // 约束函数 C_i = ρ_i / ρ_0 - 1
        float C = pi.density / rho0 - 1.0f;
        
        // 计算约束梯度 ∇C_i
        glm::vec3 gradientI(0.0f);
        float sumGradient2 = 0.0f;
        
        for (int j : m_neighbors[i]) {
            glm::vec3 r = xi - m_particles[j].predictedPosition;
            glm::vec3 gradient = spikyGradient(r) / rho0;
            sumGradient2 += glm::dot(gradient, gradient);
            gradientI += gradient;
        }
        sumGradient2 += glm::dot(gradientI, gradientI);
        
        // 计算 lambda_i = -C_i / (Σ |∇C|² + ε)
        pi.lambda = -C / (sumGradient2 + m_pbfParams.epsilon);
    }
}

void Slime::computePositionCorrection() {
    float rho0 = m_pbfParams.restDensity;
    
    for (size_t i = 0; i < m_particles.size(); ++i) {
        const glm::vec3& xi = m_particles[i].predictedPosition;
        glm::vec3 deltaPi(0.0f);
        
        for (int j : m_neighbors[i]) {
            glm::vec3 r = xi - m_particles[j].predictedPosition;
            float lambdaSum = m_particles[i].lambda + m_particles[j].lambda;
            
            // ✅ 张力修正 s_corr (防止粒子聚团)
            float wij = poly6Kernel(glm::length(r));
            float sCorr = -m_pbfParams.tensileK * std::pow(wij / m_wDeltaQPow, 
                                                            static_cast<float>(m_pbfParams.tensileN));
            
            deltaPi += (lambdaSum + sCorr) * spikyGradient(r) / rho0;
        }
        
        m_particles[i].deltaP += deltaPi;
    }
}

void Slime::applyPositionCorrection() {
    for (auto& p : m_particles) {
        p.predictedPosition += p.deltaP;
    }
}

void Slime::applyCohesionForce() {
    // ✅ 向心力：使史莱姆保持整体形状
    glm::vec3 center = getSlimeCenter();
    float cohesionStrength = m_pbfParams.cohesion;
    
    for (auto& p : m_particles) {
        glm::vec3 toCenter = center - p.predictedPosition;
        float dist = glm::length(toCenter);
        
        if (dist > EPSILON) {
            // 距离越远，向心力越强 (弹簧效果)
            float forceMagnitude = cohesionStrength * std::min(dist, m_boundaryRadius);
            glm::vec3 cohesionForce = glm::normalize(toCenter) * forceMagnitude;
            p.deltaP += cohesionForce * 0.01f;
        }
    }
}

void Slime::handleCollisions() {
    // ✅ 改进：使用物理引擎检测环境碰撞
    
    // 1. 球形边界约束 (史莱姆内部边界)
    glm::vec3 center = m_slimeCenter;
    float radius = m_boundaryRadius;
    
    for (auto& p : m_particles) {
        glm::vec3 toCenter = p.predictedPosition - center;
        float dist = glm::length(toCenter);
        
        if (dist > radius) {
            // 拉回边界
            glm::vec3 normal = glm::normalize(toCenter);
            p.predictedPosition = center + normal * radius;
            
            // 速度阻尼
            float vn = glm::dot(p.velocity, normal);
            if (vn > 0.0f) {
                p.velocity -= (1.0f + m_pbfParams.boundaryDamping) * vn * normal;
            }
        }
    }
    
    // 2. 与环境碰撞检测 (通过物理引擎射线)
    if (m_engine && m_engine->pWorld) {
        for (auto& p : m_particles) {
            // 简化处理：检测地面碰撞
            if (p.predictedPosition.y < -5.0f) {  // 假设地面在 y = -5
                p.predictedPosition.y = -5.0f;
                
                // 法向速度反弹
                if (p.velocity.y < 0.0f) {
                    p.velocity.y *= -m_pbfParams.boundaryDamping;
                    p.velocity.x *= 0.9f;  // 摩擦力
                    p.velocity.z *= 0.9f;
                }
            }
        }
    }
    
    // 3. 速度限制 (防止爆炸)
    const float maxSpeed = 20.0f;
    for (auto& p : m_particles) {
        float speed = glm::length(p.velocity);
        if (speed > maxSpeed) {
            p.velocity = glm::normalize(p.velocity) * maxSpeed;
        }
    }
}

void Slime::updateVelocities(float dt) {
    if (dt < EPSILON) return;
    
    for (auto& p : m_particles) {
        // v = (x* - x) / Δt
        p.velocity = (p.predictedPosition - p.position) / dt;
        p.position = p.predictedPosition;
    }
}

void Slime::applyXSPHViscosity() {
    // ✅ XSPH 粘度 (使邻居速度平滑)
    std::vector<glm::vec3> velocityCorrections(m_particles.size(), glm::vec3(0.0f));
    
    for (size_t i = 0; i < m_particles.size(); ++i) {
        const glm::vec3& vi = m_particles[i].velocity;
        
        for (int j : m_neighbors[i]) {
            float r = glm::length(m_particles[i].position - m_particles[j].position);
            glm::vec3 vij = m_particles[j].velocity - vi;
            velocityCorrections[i] += vij * poly6Kernel(r);
        }
        
        velocityCorrections[i] *= m_pbfParams.viscosity;
    }
    
    for (size_t i = 0; i < m_particles.size(); ++i) {
        m_particles[i].velocity += velocityCorrections[i];
    }
}

void Slime::applyVorticityConfinement(float dt) {
    // ✅ 涡量约束 (增加流体动态感)
    // 简化实现：计算涡量并施加修正力
    
    std::vector<glm::vec3> omega(m_particles.size(), glm::vec3(0.0f));
    
    // 1. 计算涡量 ω = ∇ × v
    for (size_t i = 0; i < m_particles.size(); ++i) {
        const glm::vec3& vi = m_particles[i].velocity;
        
        for (int j : m_neighbors[i]) {
            glm::vec3 r = m_particles[i].position - m_particles[j].position;
            glm::vec3 vij = m_particles[j].velocity - vi;
            omega[i] += glm::cross(vij, spikyGradient(r));
        }
    }
    
    // 2. 计算涡量梯度并施加修正力
    for (size_t i = 0; i < m_particles.size(); ++i) {
        glm::vec3 eta(0.0f);
        
        for (int j : m_neighbors[i]) {
            glm::vec3 r = m_particles[i].position - m_particles[j].position;
            eta += glm::length(omega[j]) * spikyGradient(r);
        }
        
        float etaLen = glm::length(eta);
        if (etaLen > EPSILON) {
            glm::vec3 N = eta / etaLen;
            glm::vec3 force = m_pbfParams.vorticityEpsilon * glm::cross(N, omega[i]);
            m_particles[i].velocity += force * dt;
        }
    }
}

void Slime::interactWithPhysicsWorld() {
    if (!m_rigidBody) return;
    
    // ✅ 同步刚体位置到质心
    const rp3d::Transform& transform = m_rigidBody->getTransform();
    const rp3d::Vector3& rp3dPos = transform.getPosition();
    glm::vec3 physicsCenter(rp3dPos.x, rp3dPos.y, rp3dPos.z);
    
    // ✅ 如果刚体被其他物体推动，更新粒子位置
    glm::vec3 offset = physicsCenter - m_slimeCenter;
    
    if (glm::length(offset) > EPSILON) {
        for (auto& p : m_particles) {
            p.position += offset;
        }
        m_slimeCenter = physicsCenter;
    }
    
    // ✅ 查询碰撞信息 (如果需要对粒子施加额外力)
    // ReactPhysics3D 的碰撞事件需要通过 EventListener 获取
    // 这里简化处理，后续可扩展
}

void Slime::render() const {
    if (!m_shader || !m_vao || m_particles.empty()) {
        return;
    }

    m_shader->begin();
    
    // 完全不透明渲染
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    
    m_vao->drawInstanced(m_particles.size(), m_indexCount, GL_TRIANGLES);
    
    m_shader->end();
}

void Slime::updateInstancedData() {
    if (!m_instanceVBO) return;
    
    std::vector<float> instanceData;
    instanceData.reserve(m_particles.size() * 16);
    
    for (const auto& p : m_particles) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, p.position);
        
        const float* matPtr = glm::value_ptr(model);
        instanceData.insert(instanceData.end(), matPtr, matPtr + 16);
    }
    
    m_instanceVBO->update(instanceData);
}

bool Slime::collideWith(const Object& other) const {
    // 由物理引擎处理
    return false;
}

void Slime::applyForce(const glm::vec3& force) {
    // ✅ 累积外部力，将在下一帧应用
    m_externalForce += force;
    
    // 同时应用到物理刚体
    if (m_rigidBody) {
        rp3d::Vector3 rp3dForce(force.x, force.y, force.z);
        m_rigidBody->applyWorldForceAtCenterOfMass(rp3dForce);
    }
}

glm::vec3 Slime::getSlimeCenter() const {
    if (m_particles.empty()) return m_position;
    
    glm::vec3 center(0.0f);
    for (const auto& p : m_particles) {
        center += p.position;
    }
    return center / static_cast<float>(m_particles.size());
}

// ===== 空间哈希方法 =====

int Slime::hashPosition(const glm::vec3& pos) const {
    int x = static_cast<int>(std::floor(pos.x / m_cellSize));
    int y = static_cast<int>(std::floor(pos.y / m_cellSize));
    int z = static_cast<int>(std::floor(pos.z / m_cellSize));
    
    return (x * 73856093) ^ (y * 19349663) ^ (z * 83492791);
}

void Slime::clearSpatialHash() {
    m_spatialHash.clear();
}

void Slime::insertParticle(int particleIndex) {
    const glm::vec3& pos = m_particles[particleIndex].predictedPosition;
    int hash = hashPosition(pos);
    m_spatialHash[hash].push_back(particleIndex);
}

// ===== SPH 核函数 =====

float Slime::poly6Kernel(float r) const {
    float h = m_pbfParams.smoothingRadius;
    
    if (r < 0.0f || r >= h) return 0.0f;
    
    float h2 = h * h;
    float x = h2 - r * r;
    return m_poly6Constant * x * x * x;
}

float Slime::poly6KernelNormalized(float rSquared) const {
    float h2 = m_pbfParams.smoothingRadius * m_pbfParams.smoothingRadius;
    
    if (rSquared >= h2) return 0.0f;
    
    float x = h2 - rSquared;
    return m_poly6Constant * x * x * x;
}

glm::vec3 Slime::spikyGradient(const glm::vec3& r) const {
    float h = m_pbfParams.smoothingRadius;
    float rLen = glm::length(r);
    
    if (rLen < EPSILON || rLen >= h) return glm::vec3(0.0f);
    
    float x = h - rLen;
    return m_spikyGradConstant * x * x * (r / rLen);
}

float Slime::viscosityLaplacian(float r) const {
    float h = m_pbfParams.smoothingRadius;
    
    if (r < 0.0f || r >= h) return 0.0f;
    
    return m_viscosityLapConstant * (h - r);
}
