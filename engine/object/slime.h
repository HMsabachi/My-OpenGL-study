#ifndef SLIME_H
#define SLIME_H

#include "Object.h"
#include "../../glFrameWork/core.h"
#include "../../glFrameWork/buffers.h"
#include "../../glFrameWork/shader.h"
#include "../../glFrameWork/texture.h"
#include <vector>
#include <unordered_map>
#include <reactphysics3d/reactphysics3d.h>

/**
 * @brief 史莱姆类 - 使用改进的 PBF (Position Based Fluids) 算法模拟液态效果
 * 
 * 特性:
 * - 改进的 PBF 粒子系统用于液态模拟
 * - 通过 ReactPhysics3D 刚体代表史莱姆整体与其他物体交互
 * - 粒子通过射线检测与环境碰撞
 * - 实例化渲染优化性能
 */
class Slime : public Object {
public:
    /**
     * @brief 粒子结构体
     */
    struct Particle {
        glm::vec3 position;           // 当前位置
        glm::vec3 predictedPosition;  // 预测位置（PBF）
        glm::vec3 velocity;           // 速度
        glm::vec3 deltaP;             // 位置修正累积
        float mass;                   // 质量
        float density;                // 密度
        float lambda;                 // 拉格朗日乘数（PBF）
        
        Particle() : position(0.0f), predictedPosition(0.0f), velocity(0.0f), 
                     deltaP(0.0f), mass(1.0f), density(0.0f), lambda(0.0f) {}
    };

    /**
     * @brief PBF 参数结构体 - 基于 Macklin & Müller 2013 论文
     */
    struct PBFParams {
        float restDensity = 1000.0f;      // 静止密度 (kg/m³)
        float epsilon = 600.0f;           // CFM松弛参数 (用于数值稳定性)
        float viscosity = 0.01f;          // 粘度系数 (XSPH)
        float surfaceTension = 0.0001f;   // 表面张力系数
        float cohesion = 0.01f;           // 向心力强度（史莱姆特性）
        float smoothingRadius = 0.3f;     // SPH核半径 (h)
        int solverIterations = 4;         // PBF求解器迭代次数
        float vorticityEpsilon = 0.001f;  // 涡量约束强度
        float boundaryDamping = 0.8f;     // 边界碰撞阻尼
        float deltaQ = 0.3f;              // 张力修正参数 (通常为 0.1h ~ 0.3h)
        float tensileK = 0.0001f;         // 张力不稳定性修正强度
        int tensileN = 4;                 // 张力修正指数
    };

public:
    Slime(Engine* engine,
          const glm::vec3& center = glm::vec3(0.0f),
          float radius = 1.5f,
          int particleCount = 500,
          Shader* shader = nullptr,
          GLuint texture = 0);

    ~Slime() override;

    void update(float deltaTime) override;
    void render() const override;
    bool collideWith(const Object& other) const override;
    void applyForce(const glm::vec3& force) override;

    // ===== PBF 相关方法 (基于 Position Based Dynamics) =====
    
    /**
     * @brief PBF主循环 - 按照标准算法流程
     */
    void updatePBF(float dt);
    
    /**
     * @brief 1. 应用外力 (重力 + 用户输入)
     */
    void applyExternalForces(float dt);
    
    /**
     * @brief 2. 预测位置 x* = x + Δt·v
     */
    void predictPositions(float dt);
    
    /**
     * @brief 3. 查找邻居粒子 (空间哈希加速)
     */
    void findNeighbors();
    
    /**
     * @brief 4. 求解器循环开始前重置 deltaP
     */
    void resetDeltaP();
    
    /**
     * @brief 5. 计算密度和 lambda (约束C和梯度)
     */
    void computeDensityAndLambda();
    
    /**
     * @brief 6. 计算位置修正 Δp (PBF核心)
     */
    void computePositionCorrection();
    
    /**
     * @brief 7. 应用位置修正
     */
    void applyPositionCorrection();
    
    /**
     * @brief 8. 应用向心力 (保持史莱姆整体形状)
     */
    void applyCohesionForce();
    
    /**
     * @brief 9. 更新速度 v = (x* - x) / Δt
     */
    void updateVelocities(float dt);
    
    /**
     * @brief 10. 应用 XSPH 粘度
     */
    void applyXSPHViscosity();
    
    /**
     * @brief 11. 应用涡量约束 (保持流体动态)
     */
    void applyVorticityConfinement(float dt);
    
    /**
     * @brief 12. 碰撞检测与响应
     */
    void handleCollisions();
    
    /**
     * @brief 与物理世界交互 (通过刚体)
     */
    void interactWithPhysicsWorld();

    // ===== 空间哈希加速 =====
    
    int hashPosition(const glm::vec3& pos) const;
    void clearSpatialHash();
    void insertParticle(int particleIndex);

    // ===== SPH 核函数 (Poly6, Spiky, Viscosity) =====
    
    float poly6Kernel(float r) const;
    float poly6KernelNormalized(float rSquared) const;  // 优化版本
    glm::vec3 spikyGradient(const glm::vec3& r) const;
    float viscosityLaplacian(float r) const;

    // ===== 配置与查询 =====
    
    void setPBFParams(const PBFParams& params) { m_pbfParams = params; }
    const PBFParams& getPBFParams() const { return m_pbfParams; }
    
    void setParticleRadius(float radius) { m_particleRadius = radius; updateInstancedData(); }
    float getParticleRadius() const { return m_particleRadius; }
    
    glm::vec3 getSlimeCenter() const;  // 计算质心
    
    float getParticleMass() const { return m_particleMass; }

private:
    // ===== 粒子数据 =====
    std::vector<Particle> m_particles;
    std::vector<std::vector<int>> m_neighbors;  // 邻居列表
    float m_particleMass;  // 单个粒子质量
    
    // ===== 空间哈希 =====
    std::unordered_map<int, std::vector<int>> m_spatialHash;
    float m_cellSize;  // 哈希网格大小 = smoothingRadius
    
    // ===== PBF 参数 =====
    PBFParams m_pbfParams;
    
    // ===== 核函数预计算 =====
    float m_poly6Constant;
    float m_spikyGradConstant;
    float m_viscosityLapConstant;
    float m_wDeltaQPow;  // W(deltaQ)^n
    
    // ===== 渲染相关 =====
    Shader* m_shader;
    VAO* m_vao;
    GLuint m_texture;
    float m_particleRadius;
    
    // 实例化渲染缓冲
    std::shared_ptr<Buffer<float>> m_vbo;
    std::shared_ptr<Buffer<unsigned int>> m_ebo;
    std::shared_ptr<Buffer<float>> m_instanceVBO;
    size_t m_indexCount;
    
    // ===== 物理交互 =====
    glm::vec3 m_slimeCenter;  // 史莱姆质心
    glm::vec3 m_externalForce;  // 累积的外力 (来自物理引擎或玩家输入)
    float m_boundaryRadius;   // 边界半径
    
    // ===== 辅助方法 =====
    void initParticles(const glm::vec3& center, float radius, int count);
    void initMesh();
    void updateInstancedData();
    void precomputeKernelConstants();  // 预计算核函数系数
};

#endif // SLIME_H
