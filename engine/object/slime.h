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
 * 2. 每个球体有向中心点的吸引力
 * 3. 球体之间有碰撞
 * 4. 受重力影响
 */
class Slime : public Object {
public:
    /**
     * @brief 史莱姆粒子结构（单个小球体）
     */
    struct Particle {
        glm::vec3 position;      // 位置
        glm::vec3 velocity;      // 速度
        rp3d::RigidBody* rigidBody;  // 物理刚体
        rp3d::Collider* collider;    // 碰撞体
        
        Particle() : position(0.0f), velocity(0.0f), rigidBody(nullptr), collider(nullptr) {}
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
    
    // 获取粒子信息
    const std::vector<Particle>& getParticles() const { return m_particles; }
    int getParticleCount() const { return m_particles.size(); }

private:
    // 粒子系统
    std::vector<Particle> m_particles;        // 粒子列表
    int m_numParticles;                       // 粒子数量
    float m_particleRadius;                   // 粒子半径
    glm::vec3 m_center;                       // 史莱姆中心点
    float m_maxCohesionDistance = 0.5f;       // 向心力最大作用距离

    // 力参数
    float m_cohesionForce;     // 向心力强度
    float m_damping;           // 阻尼系数（减速）
    
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
    void updateCenter();                      // 更新中心点
    void applyForces(float deltaTime);       // 应用力
    void syncParticlesFromPhysics();         // 从物理引擎同步粒子位置
};

#endif // SLIME_H
