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
#include <execution>  //  C++17 并行算法
#include <numeric>    //  std::iota
#include <thread>     //  线程支持
#include <mutex>      //  互斥锁
#include <atomic>     //  原子操作
#include <chrono>     //  ✅ 性能计时

// 常量
const float PI = 3.14159265359f;

Slime::Slime(Engine* engine, const glm::vec3& position, float radius, 
             int particleCount, Shader* particleShader, Shader* meshShader, GLuint texture)
    : Object(engine, position),
      m_slimeRadius(radius),                      // 史莱姆整体半径
      m_particleRadius(0.12f),                   // 单个粒子半径
      m_restDensity(6000.0f),                    // PBF算法的静止密度（用于约束求解）
      m_epsilon(600.0f),                         // 数值稳定性参数（避免除零）
      m_solverIterations(2),                     // 每帧约束求解迭代次数
      m_cohesionStrength(3.0f),                  // 向心力强度（保持史莱姆聚合）
      m_viscosity(0.05f),                        // 粘性系数（模拟流体内部阻力）
      m_particleShader(particleShader),          // 粒子渲染着色器
      m_meshShader(meshShader),                  // 网格渲染着色器
      m_texture(texture),                        // 纹理ID
      m_sphereIndexCount(0),                     // 球体网格索引数量（用于实例化渲染）
      m_renderMode(RenderMode::PARTICLES),       // 默认渲染模式：粒子球体
      m_meshResolution(28),                      // 密度场网格分辨率（用于Marching Cubes）
      m_isoLevel(0.5f),                         // 等值面阈值（密度大于此值为实心）
      m_blurIterations(6),                       // 密度场模糊迭代次数（使表面更光滑）
      m_meshUpdateTimer(0.0f),                   // 网格更新计时器
      m_meshUpdateInterval(0.01f),              // 网格更新间隔（0.005秒 = 每秒更新200次）
      m_minComponentSize(2),                     // 最小连通块大小（少于5个粒子的块将被忽略）
      m_particleVAO(nullptr),                    // 粒子VAO（Vertex Array Object）
      m_meshVAO(nullptr),                        // 网格VAO（保留用于向后兼容）
      m_marchingCubes(nullptr),                  // Marching Cubes算法实例（用于生成网格）
      m_connectedComponents(nullptr)             // 连通域分析器（用于识别独立的史莱姆块）
{
    // 初始化粒子
    m_particles.resize(particleCount);
    m_neighbors.resize(particleCount);
    
    //  创建粒子索引数组（用于并行遍历）
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
    
    // ✅ 初始化 Marching Cubes 和连通域分析器
    m_marchingCubes = new MarchingCubes();
    m_connectedComponents = new ConnectedComponents();
    
    //  更新日志输出
    std::cout << "[Slime] 史莱姆创建成功：" << particleCount << " 个粒子 | 并行计算：启用" 
              << " | CPU 核心数：" << std::thread::hardware_concurrency() 
              << " | 网格分辨率：" << m_meshResolution 
              << " | 连通域分析：启用" << std::endl;
}

Slime::~Slime() {
    delete m_particleVAO;
    delete m_meshVAO;
    delete m_marchingCubes;
    delete m_connectedComponents;
    
    // ✅ 清理多块网格
    for (auto& compMesh : m_componentMeshes) {
        delete compMesh.vao;
    }
}

void Slime::initRenderData() {
    // ===== 粒子渲染数据 =====
    // 创建一个小球体网格作为粒子的基础模型
    widgets::SphereData sphereData = widgets::createSphere(m_particleRadius, 8, 6);
    
    m_sphereVBO = std::make_shared<Buffer<float>>(sphereData.vertices, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    m_sphereEBO = std::make_shared<Buffer<unsigned int>>(sphereData.indices, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
    m_sphereIndexCount = sphereData.indices.size();
    
    // 创建实例化矩阵缓冲（作为float数组）
    std::vector<float> instanceData(m_particles.size() * 16);  // mat4 = 16个float
    for (size_t i = 0; i < m_particles.size(); ++i) {
        glm::mat4 matrix = glm::translate(glm::mat4(1.0f), m_particles[i].position);
        memcpy(&instanceData[i * 16], glm::value_ptr(matrix), 16 * sizeof(float));
    }
    
    m_instanceVBO = std::make_shared<Buffer<float>>(instanceData, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
    
    // 配置粒子VAO
    m_particleVAO = new VAO();
    m_particleVAO->addVBO(*m_sphereVBO, "3f 3f 2f", GL_FALSE, 0);  // 顶点数据: pos, normal, texCoord
    m_particleVAO->addInstancedVBO(*m_instanceVBO, "4f 4f 4f 4f", 3, 1);  // 从location 3开始
    m_particleVAO->addEBO(*m_sphereEBO);
    
    // ✅ 网格渲染数据（不再需要预分配大缓冲区，每个块独立创建）
    // 保留这些成员变量用于向后兼容，但不使用
    m_meshVAO = nullptr;
    m_meshVBO = nullptr;
    m_meshEBO = nullptr;
    m_meshIndexCount = 0;
    
    std::cout << "[Slime] 渲染数据初始化完成 | 粒子：" << m_particles.size() 
              << " | 网格：动态多块生成" << std::endl;
}

void Slime::update(float deltaTime) {
    // 限制时间步长，避免不稳定
    const float maxDt = 0.016f;  // 约60fps
    deltaTime = std::min(deltaTime, maxDt);
    
    //  PBF模拟步骤（纯并行优化）
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
    
    //  使用物理查询进行碰撞检测
    handlePhysicsCollisions();
    
    // 更新渲染数据
    if (m_renderMode == RenderMode::PARTICLES) {
        updateInstanceBuffer();
    } else {
        // 网格模式：定期更新网格
        m_meshUpdateTimer += deltaTime;
        if (m_meshUpdateTimer >= m_meshUpdateInterval) {
            generateMeshes();  // ✅ 改为多块网格生成
            updateMeshBuffers();
            m_meshUpdateTimer = 0.0f;
        }
    }
    
    //  更新质心位置（用于相机跟踪）
    m_position = getCenterOfMass();
}


//  并行优化：施加外力（删除串行代码）
void Slime::applyExternalForces(float dt) {
    const glm::vec3 gravity(0.0f, -9.81f, 0.0f);
    
    //  纯并行处理
    std::for_each(std::execution::par_unseq, m_particles.begin(), m_particles.end(),
        [gravity](Particle& particle) {
            particle.force += gravity;
        });
}

//  并行优化：预测位置（删除串行代码）
void Slime::predictPositions(float dt) {
    
    std::for_each(std::execution::par_unseq, m_particles.begin(), m_particles.end(),
        [dt](Particle& particle) {
            particle.velocity += particle.force * dt;
            particle.predictedPos = particle.position + particle.velocity * dt;
            particle.force = glm::vec3(0.0f);
        });
}

//  并行优化：构建空间哈希（Lock-Free 优化）
void Slime::buildSpatialHash() {
    m_spatialHash.clear();
    
    //  第一步：并行计算所有粒子的哈希键
    std::vector<std::pair<int, int>> hashKeyPairs(m_particles.size());
    
    std::for_each(std::execution::par_unseq, m_particleIndices.begin(), m_particleIndices.end(),
        [this, &hashKeyPairs](int i) {
            int key = getHashKey(m_particles[i].predictedPos);
            hashKeyPairs[i] = {key, i};
        });
    
    //  第二步：串行合并到哈希表（这部分很快，不需要并行）
    for (const auto& [key, idx] : hashKeyPairs) {
        m_spatialHash[key].push_back(idx);
    }
}

//  并行优化：更新邻居（删除串行代码）
void Slime::updateNeighbors() {
    const float h = m_particleRadius * 4.0f;
    const float h_sq = h * h;  //  优化：避免重复计算平方根
    
    //  纯并行处理
    std::for_each(std::execution::par, m_particleIndices.begin(), m_particleIndices.end(),
        [this, h_sq](int i) {
            m_neighbors[i].clear();
            std::vector<int> candidates = getNeighbors(m_particles[i].predictedPos);
            
            //  预分配空间，减少动态分配
            m_neighbors[i].reserve(32);
            
            for (int j : candidates) {
                if (i == j) continue;
                
                //  优化：先用平方距离判断，避免 sqrt
                glm::vec3 diff = m_particles[i].predictedPos - m_particles[j].predictedPos;
                float dist_sq = glm::dot(diff, diff);
                
                if (dist_sq < h_sq) {
                    m_neighbors[i].push_back(j);
                }
            }
        });
}

//  并行优化：约束求解（删除串行代码）
void Slime::solveConstraints() {
    //  第一步：并行计算 lambda
    std::for_each(std::execution::par, m_particleIndices.begin(), m_particleIndices.end(),
        [this](int i) {
            m_particles[i].lambda = computeLambda(i);
        });
    
    //  第二步：并行计算位置修正
    std::for_each(std::execution::par, m_particleIndices.begin(), m_particleIndices.end(),
        [this](int i) {
            m_particles[i].deltaPos = computeDeltaP(i);
        });
    
    //  第三步：并行应用位置修正（使用 par_unseq 向量化）
    std::for_each(std::execution::par_unseq, m_particles.begin(), m_particles.end(),
        [](Particle& particle) {
            particle.predictedPos += particle.deltaPos;
        });
}

//  并行优化：更新速度（删除串行代码）
void Slime::updateVelocities(float dt) {
    const float invDt = 1.0f / dt;  //  优化：避免除法
    
    std::for_each(std::execution::par_unseq, m_particles.begin(), m_particles.end(),
        [invDt](Particle& particle) {
            particle.velocity = (particle.predictedPos - particle.position) * invDt;
            particle.position = particle.predictedPos;
        });
}

//  并行优化：向心力
void Slime::applyCohesionForce() {
    const glm::vec3 centerOfMass = getCenterOfMass();
    const glm::vec3 targetCenter = centerOfMass + glm::vec3(0.0f, m_slimeRadius * 0.2f, 0.0f);
    
    // 预计算常量
    const float radiusThreshold = m_slimeRadius * 0.5f;
    const float invRadius = 1.0f / m_slimeRadius;
    const float idealDist = m_particleRadius * 4.2f;
    const float maxAttractionDist = m_particleRadius * 5.5f;
    const float attractionRange = maxAttractionDist - idealDist;
    const float maxForce = m_cohesionStrength * 3.0f;
    
    //  纯并行处理
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

//  并行优化：粘性（删除串行代码）
void Slime::applyViscosity() {
    std::for_each(std::execution::par, m_particleIndices.begin(), m_particleIndices.end(),
        [this](int i) {
            glm::vec3 velocityChange(0.0f);
            int neighborCount = m_neighbors[i].size();
            
            if (neighborCount == 0) return;
            
            //  优化：累加邻居速度差
            for (int neighborIdx : m_neighbors[i]) {
                velocityChange += m_particles[neighborIdx].velocity - m_particles[i].velocity;
            }
            
            //  优化：一次除法
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
    neighbors.reserve(64);  //  预分配空间
    
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

//  优化：并行碰撞检测（分块处理）
void Slime::handlePhysicsCollisions() {
    if (!m_engine) return;
    
    auto* world = m_engine->getPhysicsWorld();
    if (!world) return;
    
    const float checkDistance = m_particleRadius * 2.0f;
    const float restitution = 0.3f;
    const float friction = 0.4f;
    const float minSpeed = 0.01f;  // 最小速度阈值
    
    //  并行处理碰撞检测
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

//  优化：并行更新实例缓冲
void Slime::updateInstanceBuffer() {
    std::vector<float> instanceData(m_particles.size() * 16);
    
    //  并行生成矩阵数据
    std::for_each(std::execution::par_unseq, m_particleIndices.begin(), m_particleIndices.end(),
        [this, &instanceData](int i) {
            glm::mat4 matrix = glm::translate(glm::mat4(1.0f), m_particles[i].position);
            memcpy(&instanceData[i * 16], glm::value_ptr(matrix), 16 * sizeof(float));
        });
    
    //  使用封装的 update 方法更新GPU缓冲
    m_instanceVBO->update(instanceData, 0);
}

void Slime::render() const {
    if (m_renderMode == RenderMode::PARTICLES) {
        // 粒子球体模式
        if (!m_particleShader) return;
        
        m_particleShader->begin();
        
        // 绑定纹理（如果有）
        if (m_texture > 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_texture);
        }
        
        // 设置史莱姆颜色
        m_particleShader->set("uSlimeColor", glm::vec3(0.3f, 1.0f, 0.5f));
        
        // 实例化绘制所有粒子
        m_particleVAO->drawInstanced(m_particles.size(), m_sphereIndexCount);
        
        m_particleShader->end();
    } else {
        // ✅ 网格模式：渲染所有独立块
        if (!m_meshShader || m_componentMeshes.empty()) return;
        
        // 确保深度测试启用
        glEnable(GL_DEPTH_TEST);
        
        // 启用混合以支持半透明
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        m_meshShader->begin();
        
        // 设置模型矩阵（单位矩阵，因为顶点已经在世界空间）
        m_meshShader->set("uModel", glm::mat4(1.0f));
        
        // 设置史莱姆颜色
        m_meshShader->set("uSlimeColor", glm::vec3(0.3f, 1.0f, 0.5f));
        
        // ✅ 渲染每个独立的网格块
        for (const auto& compMesh : m_componentMeshes) {
            if (compMesh.indexCount > 0 && compMesh.vao) {
                compMesh.vao->draw(GL_TRIANGLES, compMesh.indexCount);
            }
        }
        
        m_meshShader->end();
        
        glDisable(GL_BLEND);
    }
}

void Slime::toggleRenderMode() {
    if (m_renderMode == RenderMode::PARTICLES) {
        m_renderMode = RenderMode::MESH;
        std::cout << "[Slime] 切换到网格渲染模式" << std::endl;
    } else {
        m_renderMode = RenderMode::PARTICLES;
        std::cout << "[Slime] 切换到粒子渲染模式" << std::endl;
    }
}

// 多块网格生成
void Slime::generateMeshes() {
    // 性能计时
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // 1. 提取粒子位置
    std::vector<glm::vec3> positions(m_particles.size());
    std::transform(std::execution::par_unseq,
                   m_particles.begin(), m_particles.end(),
                   positions.begin(),
                   [](const Particle& p) { return p.position; });
    
    // 2. 使用连通域分析将粒子分组
    float searchRadius = m_particleRadius * 4.0f;  // 与邻居搜索半径一致
    
    auto connStart = std::chrono::high_resolution_clock::now();
    std::vector<ComponentInfo> components = 
        m_connectedComponents->analyzeComponents(positions, searchRadius, m_minComponentSize);
    auto connEnd = std::chrono::high_resolution_clock::now();
    
    // 3. 清理旧的网格
    for (auto& compMesh : m_componentMeshes) {
        delete compMesh.vao;
    }
    m_componentMeshes.clear();
    
    if (components.empty()) {
        return;  // 没有足够大的连通块
    }
    
    // 4. 为每个连通块生成独立的网格
    auto meshStart = std::chrono::high_resolution_clock::now();
    
    // ✅ 并行生成所有块的网格数据
    std::vector<MeshData> meshDataList(components.size());
    std::vector<int> compIndices(components.size());
    std::iota(compIndices.begin(), compIndices.end(), 0);
    
    std::for_each(std::execution::par, compIndices.begin(), compIndices.end(),
        [this, &components, &meshDataList](int compIdx) {
            const auto& component = components[compIdx];
            
            // 为该块创建密度场
            DensityField densityField(component.boundsMin, component.boundsMax, m_meshResolution);
            
            // 构建密度场（只使用该块的粒子）
            densityField.buildFromParticles(component.particlePositions, m_particleRadius);
            
            // 应用模糊
            densityField.applyBlur(m_blurIterations);
            
            // 使用 Marching Cubes 生成网格
            meshDataList[compIdx] = m_marchingCubes->generateMesh(densityField, m_isoLevel);
        });
    
    auto meshEnd = std::chrono::high_resolution_clock::now();
    
    // 5. 串行创建 GPU 缓冲区（OpenGL 调用必须在主线程）
    auto bufferStart = std::chrono::high_resolution_clock::now();
    
    for (size_t compIdx = 0; compIdx < components.size(); ++compIdx) {
        const auto& meshData = meshDataList[compIdx];
        
        // 如果网格为空，跳过
        if (meshData.vertexCount() == 0) {
            continue;
        }
        
        // 创建组件网格
        ComponentMesh compMesh;
        compMesh.meshData = meshData;  // 复制而非移动，因为我们需要保留数据
        compMesh.indexCount = compMesh.meshData.indices.size();
        
        // 准备顶点数据（位置 + 法线）
        const size_t vertexCapacity = compMesh.meshData.vertexCount() * 6;  // pos + normal
        std::vector<float> vertexData(vertexCapacity);
        
        // ✅ 并行准备顶点数据
        std::vector<int> vertexIndices(compMesh.meshData.vertexCount());
        std::iota(vertexIndices.begin(), vertexIndices.end(), 0);
        
        std::for_each(std::execution::par_unseq, vertexIndices.begin(), vertexIndices.end(),
            [&compMesh, &vertexData](int i) {
                const auto& pos = compMesh.meshData.positions[i];
                const auto& normal = compMesh.meshData.normals[i];
                
                size_t offset = i * 6;
                vertexData[offset + 0] = pos.x;
                vertexData[offset + 1] = pos.y;
                vertexData[offset + 2] = pos.z;
                vertexData[offset + 3] = normal.x;
                vertexData[offset + 4] = normal.y;
                vertexData[offset + 5] = normal.z;
            });
        
        // 使用封装的 Buffer 创建 VBO 和 EBO
        compMesh.vbo = std::make_shared<Buffer<float>>(vertexData, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
        compMesh.ebo = std::make_shared<Buffer<unsigned int>>(
            compMesh.meshData.indices, GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW
        );
        
        // 配置 VAO
        compMesh.vao = new VAO();
        compMesh.vao->addVBO(*compMesh.vbo, "3f 3f", GL_FALSE, 0);  // pos + normal
        compMesh.vao->addEBO(*compMesh.ebo);
        
        m_componentMeshes.push_back(std::move(compMesh));
    }
    
    auto bufferEnd = std::chrono::high_resolution_clock::now();
    auto endTime = std::chrono::high_resolution_clock::now();
    
    // 性能统计（每10帧输出一次）
    static int frameCounter = 0;
    if (++frameCounter >= 120) {
        frameCounter = 0;
        
        auto connTime = std::chrono::duration_cast<std::chrono::microseconds>(connEnd - connStart).count();
        auto meshTime = std::chrono::duration_cast<std::chrono::microseconds>(meshEnd - meshStart).count();
        auto bufferTime = std::chrono::duration_cast<std::chrono::microseconds>(bufferEnd - bufferStart).count();
        auto totalTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        
        size_t totalVertices = 0;
        size_t totalTriangles = 0;
        for (const auto& compMesh : m_componentMeshes) {
            totalVertices += compMesh.meshData.vertexCount();
            totalTriangles += compMesh.meshData.triangleCount();
        }
        
        std::cout << "[Slime] 网格生成性能 - 总时间: " << (totalTime / 1000.0f) << "ms"
                  << " | 连通域: " << (connTime / 1000.0f) << "ms"
                  << " | 网格: " << (meshTime / 1000.0f) << "ms"
                  << " | 缓冲: " << (bufferTime / 1000.0f) << "ms"
                  << " | 块数: " << m_componentMeshes.size()
                  << " | 顶点: " << totalVertices
                  << " | 三角形: " << totalTriangles << std::endl;
    }
}

// ✅ 修改：updateMeshBuffers 不再需要（缓冲区在 generateMeshes 中创建）
void Slime::updateMeshBuffers() {
    // 缓冲区已在 generateMeshes() 中创建
    // 这个函数保持为空或输出调试信息
    if (m_componentMeshes.empty()) {
        std::cout << "[Slime] 警告：没有生成任何网格块" << std::endl;
    } else {
        size_t totalVertices = 0;
        size_t totalTriangles = 0;
        for (const auto& compMesh : m_componentMeshes) {
            totalVertices += compMesh.meshData.vertexCount();
            totalTriangles += compMesh.meshData.triangleCount();
        }
        //std::cout << "[Slime] 网格更新：" << m_componentMeshes.size() << " 块, "
                  //<< totalVertices << " 总顶点, " 
                  //<< totalTriangles << " 总三角形" << std::endl;
    }
}

bool Slime::collideWith(const Object& other) const {
    // PBF粒子系统使用自己的碰撞检测
    return false;
}

void Slime::applyForce(const glm::vec3& force) {
    const float forcePerParticle = 1.0f / static_cast<float>(m_particles.size());
    const glm::vec3 distributedForce = force * forcePerParticle;
    
    //  并行施加力
    std::for_each(std::execution::par_unseq, m_particles.begin(), m_particles.end(),
        [distributedForce](Particle& particle) {
            particle.force += distributedForce;
        });
}

glm::vec3 Slime::getCenterOfMass() const {
    //  使用并行循环累加位置，然后求平均
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
