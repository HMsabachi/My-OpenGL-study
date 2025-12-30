#ifndef SLIME_H
#define SLIME_H

#include "Object.h"
#include "../../glFrameWork/core.h"
#include "../../glFrameWork/buffers.h"
#include "../../glFrameWork/shader.h"
#include <vector>
#include <memory>

/**
 * @brief 史莱姆类 - 使用粒子系统模拟液体效果
 * 
 * 实现原理：
 * 1. 由多个小球体组成
 * 2. 动态选择中心粒子（最接近质心的粒子）
 * 3. 力从中心粒子向外传播，距离越远力越小
 * 4. 球体之间有碰撞和向心力
 */
class Slime : public Object {
public:
    /**
     * @brief 史莱姆粒子结构（单个小球体）
     */
    struct Particle {
        glm::vec3 position;      // 位置
        glm::vec3 velocity;      // 速度
        rp3d::RigidBody* rigidBody = nullptr;  // 物理刚体
        rp3d::Collider* collider = nullptr;    // 碰撞体
        bool isCore = false;  // ✅ 标记是否为核心粒子
        
        Particle() : position(0.0f), velocity(0.0f), rigidBody(nullptr), collider(nullptr), isCore(false) {}
    };

    /**
     * @brief 构造函数
     * @param engine 游戏引擎实例
     * @param center 史莱姆中心位置
     * @param numParticles 粒子数量
     * @param particleRadius 每个粒子的半径
     * @param initialRadius 初始生成半径
     * @param shader 渲染着色器
     * @param texture 纹理ID
     */
    Slime(Engine* engine,
          const glm::vec3& center = glm::vec3(0.0f),
          int numParticles = 20,
          float particleRadius = 0.15f,
          float initialRadius = 1.0f,
          Shader* shader = nullptr,
          GLuint texture = 0);

    ~Slime() override;

    void update(float deltaTime) override;
    void render() const override;
    bool collideWith(const Object& other) const override;

    // 参数设置
    void setCohesionForce(float force) { m_cohesionForce = force; }
    void setDamping(float damping) { m_damping = damping; }
	void setMaxCohesionDistance(float distance) { m_maxCohesionDistance = distance; }
    void setForceRadius(float radius) { m_forceRadius = radius; }  // 设置力的作用半径
    void setVerticalBias(float bias) { m_verticalBias = bias; }    // ✅ 设置垂直偏好（权重）
    void setGravityBoost(float boost) { m_gravityBoost = boost; }  // ✅ 设置额外重力强度
    void setRepulsionForce(float force) { m_repulsionForce = force; }     // 设置粒子间排斥力
    void setRepulsionRadius(float radius) { m_repulsionRadius = radius; }    // 设置排斥力作用半径
    void setSurfaceTension(float tension) { m_surfaceTension = tension; }     // 设置表面张力
    void setViscosity(float viscosity) { m_viscosity = viscosity; }          // 设置流体粘度
    void setRestDensity(float density) { m_restDensity = density; }          // 设置静止密度
    
    // ✅ 施加外力（区域性，只影响中心附近的粒子）
    void applyForce(const glm::vec3& force) override;
    
    // 获取粒子信息
    const std::vector<Particle>& getParticles() const { return m_particles; }
    int getParticleCount() const { return m_particles.size(); }
    
    // 获取中心粒子索引
    int getCoreParticleIndex() const { return m_coreParticleIndex; }
    
    // ✅ 调试可视化开关
    void setDebugVisualization(bool enable) { m_debugVisualization = enable; }
    bool isDebugVisualizationEnabled() const { return m_debugVisualization; }
    
    // ✅ 未来：分裂功能接口（预留）
    // Slime* split(float splitRatio = 0.5f);  // 按比例分裂史莱姆

private:
    // 粒子系统
    std::vector<Particle> m_particles;        // 粒子列表
    int m_numParticles;                       // 粒子数量
    float m_particleRadius;                   // 粒子半径
    glm::vec3 m_center;                       // 史莱姆几何中心（质心）
    int m_coreParticleIndex;                  // ✅ 中心粒子索引
    float m_maxCohesionDistance = 0.5f;       // 向心力最大作用距离
    float m_forceRadius = 1.5f;               // ✅ 外力作用半径（相对于中心粒子）
    float m_verticalBias = 2.0f;              // ✅ 垂直偏好权重（越大越偏向下方）
    float m_gravityBoost = 5.0f;              // ✅ 额外重力强度（施加给高于质心的粒子）
    float m_repulsionForce;     // 粒子间排斥力
    float m_repulsionRadius;    // 排斥力作用半径
    float m_surfaceTension;     // 表面张力（粒子密度低时的吸引力）
    float m_viscosity;          // 流体粘度（速度同步强度）
    float m_restDensity;        // 静止密度（目标粒子间距）
    bool m_debugVisualization = false;        // ✅ 调试可视化开关

    // 力参数
    float m_cohesionForce;      // 向心力强度
    float m_damping;            // 阻尼系数（减速）
    
    // 渲染相关
    Shader* m_shader;
    GLuint m_texture;
    VAO* m_particleVAO;                      // 单个球体的 VAO
    size_t m_indexCount;                     // 索引数量
    
    // 缓冲区（保持存活）
    std::shared_ptr<Buffer<float>> m_vbo;
    std::shared_ptr<Buffer<unsigned int>> m_ebo;
    
    // 物理形状（共享）
    rp3d::CollisionShape* m_sphereShape;

    // 初始化方法
    void initParticles(float initialRadius);
    void initMesh();
    void cleanupPhysics();
    
    // 更新方法
    void updateCenter();                      // 更新几何中心（质心）
    void updateCoreParticle();                // ✅ 更新中心粒子
    void applyForces(float deltaTime);        // 应用向心力
    void syncParticlesFromPhysics();          // 从物理引擎同步粒子位置
    
    // ✅ 计算粒子距离中心粒子的距离
    float getDistanceFromCore(int particleIndex) const;

    // ✅ 新增力学计算方法
    void applyRepulsionForces(float deltaTime);     // 应用排斥力
    void applySurfaceTension(float deltaTime);      // 应用表面张力
    void applyViscosity(float deltaTime);           // 应用粘度
    float calculateLocalDensity(int particleIndex); // 计算局部密度
};

#endif // SLIME_H
