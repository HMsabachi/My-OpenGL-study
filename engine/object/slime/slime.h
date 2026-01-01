// slime.h
#ifndef SLIME_H
#define SLIME_H

#include "../object.h"
#include "../../glFrameWork/shader.h"
#include "../../glFrameWork/buffers.h"
#include "densityField.h"
#include "marchingCubes.h"
#include "connectedComponents.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <unordered_map>

/**
 * 史莱姆类 - 基于PBF（Position Based Fluids）算法的液体模拟
 * 使用物理查询API检测碰撞，保持流动性
 * ✅ 使用 CPU 多线程并行计算，榨干CPU性能
 * ✅ 支持粒子球体和动态网格两种渲染模式
 * ✅ 支持史莱姆分裂后每个独立块单独渲染
 */
class Slime : public Object {
public:
    // 渲染模式枚举
    enum class RenderMode {
        PARTICLES,  // 粒子球体模式
        MESH        // 动态网格模式
    };

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
     * @param particleShader 粒子渲染shader
     * @param meshShader 网格渲染shader
     * @param texture 纹理ID
     */
    Slime(Engine* engine, const glm::vec3& position, float radius, 
          int particleCount, Shader* particleShader, Shader* meshShader, GLuint texture);
    
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
    
    // ✅ 访问粒子数据的接口
    const std::vector<Particle>& getParticles() const { return m_particles; }
    Particle& getParticleMutable(int index) { return m_particles[index]; }
    const std::vector<std::vector<int>>& getNeighbors() const { return m_neighbors; }
    float getSlimeRadius() const { return m_slimeRadius; }
    
    // ✅ 渲染模式控制
    void setRenderMode(RenderMode mode) { m_renderMode = mode; }
    RenderMode getRenderMode() const { return m_renderMode; }
    void toggleRenderMode();
    
    // ✅ 网格生成参数
    void setMeshResolution(int resolution) { m_meshResolution = resolution; }
    void setIsoLevel(float level) { m_isoLevel = level; }
    void setBlurIterations(int iterations) { m_blurIterations = iterations; }
    
    // ✅ 连通域分析参数
    void setMinComponentSize(int size) { m_minComponentSize = size; }
    int getComponentCount() const { return m_componentMeshes.size(); }

private:
    // PBF算法步骤
    void applyExternalForces(float dt);
    void predictPositions(float dt);
    void updateNeighbors();
    void solveConstraints();
    void updateVelocities(float dt);
    void applyCohesionForce();
    void applyViscosity();
    void handlePhysicsCollisions();
    
    // 密度计算
    float computeDensity(int particleIdx);
    float computeLambda(int particleIdx);
    glm::vec3 computeDeltaP(int particleIdx);
    
    // 空间哈希
    void buildSpatialHash();
    int getHashKey(const glm::vec3& pos);
    std::vector<int> getNeighbors(const glm::vec3& pos);
    
    // 渲染相关
    void initRenderData();
    void updateInstanceBuffer();
    
    // ✅ 多块网格生成
    void generateMeshes();
    void updateMeshBuffers();
    
    // SPH核函数
    float poly6Kernel(float r, float h);
    glm::vec3 spikyGradient(const glm::vec3& r, float h);
    
private:
    // PBF 参数
    float m_slimeRadius;
    float m_particleRadius;
    float m_restDensity;
    float m_epsilon;
    int m_solverIterations;
    float m_cohesionStrength;
    float m_viscosity;
    
    // 粒子数据
    std::vector<Particle> m_particles;
    std::vector<std::vector<int>> m_neighbors;
    std::vector<int> m_particleIndices;
    
    // 空间哈希
    std::unordered_map<int, std::vector<int>> m_spatialHash;
    float m_cellSize;
    
    // 渲染数据（粒子模式）
    Shader* m_particleShader;
    GLuint m_texture;
    VAO* m_particleVAO;
    std::shared_ptr<Buffer<float>> m_sphereVBO;
    std::shared_ptr<Buffer<unsigned int>> m_sphereEBO;
    std::shared_ptr<Buffer<float>> m_instanceVBO;
    size_t m_sphereIndexCount;
    
    // ✅ 网格渲染数据（保留单一VAO用于初始化）
    Shader* m_meshShader;
    VAO* m_meshVAO;
    std::shared_ptr<Buffer<float>> m_meshVBO;
    std::shared_ptr<Buffer<unsigned int>> m_meshEBO;
    size_t m_meshIndexCount;  // 不再使用，保留向后兼容
    
    // ✅ 多块网格渲染数据
    struct ComponentMesh {
        MeshData meshData;
        std::shared_ptr<Buffer<float>> vbo;
        std::shared_ptr<Buffer<unsigned int>> ebo;
        VAO* vao;
        size_t indexCount;
        
        ComponentMesh() : vao(nullptr), indexCount(0) {}
        ~ComponentMesh() { 
            // VAO 会在 Slime::~Slime() 中统一删除，这里不需要删除
        }
    };
    
    std::vector<ComponentMesh> m_componentMeshes;  // 多个独立块的网格
    
    // ✅ 网格生成工具
    RenderMode m_renderMode;
    MarchingCubes* m_marchingCubes;
    ConnectedComponents* m_connectedComponents;
    
    // 网格生成参数
    int m_meshResolution;       // 密度场分辨率
    float m_isoLevel;           // 等值面阈值
    int m_blurIterations;       // 模糊迭代次数
    float m_meshUpdateTimer;    // 网格更新计时器
    float m_meshUpdateInterval; // 网格更新间隔（秒）
    int m_minComponentSize;     // 最小连通块大小（粒子数）
};

#endif // SLIME_H
