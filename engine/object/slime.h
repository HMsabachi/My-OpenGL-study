// slime.h
#ifndef SLIME_H
#define SLIME_H

#include "object.h"
#include "../../glFrameWork/shader.h"
#include "../../glFrameWork/buffers.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <unordered_map>

/**
 * 史莱姆类 - 基于PBF（Position Based Fluids）算法的液体模拟
 */
class Slime : public Object {
public:
    // 粒子结构
    struct Particle {
        glm::vec3 position;      // 当前位置
        glm::vec3 predictedPos;  // 预测位置
        glm::vec3 velocity;      // 速度
        glm::vec3 force;         // 受力
        float lambda;            // 拉格朗日乘数
        glm::vec3 deltaPos;      // 位置修正
    };
    
    /**
     * 构造函数
     * @param engine 引擎指针
     * @param position 初始位置
     * @param radius 史莱姆半径
     * @param particleCount 粒子数量
     * @param shader 渲染shader
     * @param texture 纹理ID
     */
    Slime(Engine* engine, const glm::vec3& position, float radius, 
          int particleCount, Shader* shader, GLuint texture);
    
    virtual ~Slime();
    
    virtual void update(float deltaTime) override;
    virtual void render() const override;
    virtual bool collideWith(const Object& other) const override;
    virtual void applyForce(const glm::vec3& force) override;
    
    // 获取史莱姆中心位置（质心）
    glm::vec3 getCenterOfMass() const;
    
    // PBF参数设置
    void setRestDensity(float density) { m_restDensity = density; }
    void setParticleRadius(float radius) { m_particleRadius = radius; }
    void setCohesionStrength(float strength) { m_cohesionStrength = strength; }

	float getCohesionStrength() const { return m_cohesionStrength; }
    
private:
    // PBF算法步骤
    void applyExternalForces(float dt);
    void predictPositions(float dt);
    void updateNeighbors();
    void solveConstraints();
    void updateVelocities(float dt);
    void applyCohesionForce();  // 向心力
    void applyViscosity();
    
    // 辅助函数
    float computeDensity(int particleIdx);
    float computeLambda(int particleIdx);
    glm::vec3 computeDeltaP(int particleIdx);
    glm::vec3 spikyGradient(const glm::vec3& r, float h);
    float poly6Kernel(float r, float h);
    
    // 网格空间加速邻居搜索
    void buildSpatialHash();
    std::vector<int> getNeighbors(const glm::vec3& pos);
    int getHashKey(const glm::vec3& pos);
    
    // 渲染相关
    void initRenderData();
    void updateInstanceBuffer();
    
    // 粒子数据
    std::vector<Particle> m_particles;
    std::vector<std::vector<int>> m_neighbors;  // 每个粒子的邻居列表
    
    // 空间哈希表（用于加速邻居搜索）
    std::unordered_map<int, std::vector<int>> m_spatialHash;
    float m_cellSize;
    
    // PBF参数
    float m_slimeRadius;          // 史莱姆整体半径
    float m_particleRadius;       // 单个粒子半径
    float m_restDensity;          // 静止密度
    float m_epsilon;              // 松弛参数
    int m_solverIterations;       // 求解器迭代次数
    float m_cohesionStrength;     // 向心力强度
    float m_viscosity;            // 粘性系数
    
    // 渲染数据
    Shader* m_shader;
    GLuint m_texture;
    VAO* m_vao;
    std::shared_ptr<Buffer<float>> m_sphereVBO;
    std::shared_ptr<Buffer<unsigned int>> m_sphereEBO;
    std::shared_ptr<Buffer<float>> m_instanceVBO;  // 实例化缓冲（float数组）
    int m_sphereIndexCount;
    
    // 碰撞检测
    void handleCollisions();
    void handleObjectCollisions();  // 与ReactPhysics3D对象碰撞
};

#endif // SLIME_H
