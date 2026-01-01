// slimeController.h
#ifndef SLIME_CONTROLLER_H
#define SLIME_CONTROLLER_H

#include <glm/glm.hpp>
#include <vector>

class Slime;

/**
 * @brief 史莱姆控制器 - 管理单个史莱姆的凝聚行为
 * 
 * 负责：
 * - 检测粒子集群（分裂检测）
 * - 为每个集群独立施加向心力
 * - 管理主集群（最大的集群）
 */
class SlimeController {
public:
    SlimeController(Slime* slime);
    ~SlimeController() = default;

    /**
     * @brief 更新史莱姆控制逻辑
     * @param deltaTime 时间步长
     */
    void update(float deltaTime);

    /**
     * @brief 施加移动力到主集群（最大的集群）
     * @param force 施加的力
     */
    void applyForceToMainCluster(const glm::vec3& force);

    /**
     * @brief 获取主集群的质心
     * @return 主集群质心位置
     */
    glm::vec3 getMainClusterCenter() const;

    /**
     * @brief 获取主集群的粒子数量
     * @return 粒子数量
     */
    int getMainClusterSize() const;

    /**
     * @brief 设置凝聚力生效范围
     * @param range 范围半径
     */
    void setCohesionRange(float range) { m_cohesionRange = range; }
    float getCohesionRange() const { return m_cohesionRange; }

    /**
     * @brief 设置最小集群大小（小于此值的集群会被忽略）
     * @param size 最小粒子数
     */
    void setMinClusterSize(int size) { m_minClusterSize = size; }
    int getMinClusterSize() const { return m_minClusterSize; }

    /**
     * @brief 获取当前集群数量
     * @return 集群数量
     */
    int getClusterCount() const { return m_clusters.size(); }

private:
    Slime* m_slime;  // 管理的史莱姆对象

    // 集群信息
    struct Cluster {
        std::vector<int> particleIndices;  // 集群内的粒子索引
        glm::vec3 center;                   // 集群质心
        float radius;                       // 集群半径
        
        int size() const { return particleIndices.size(); }
    };

    std::vector<Cluster> m_clusters;       // 所有集群
    int m_mainClusterIndex;                // 主集群索引（最大的集群）

    // 参数
    float m_cohesionRange;                 // 凝聚力生效范围
    int m_minClusterSize;                  // 最小集群大小

    // 内部方法
    
    /**
     * @brief 检测并划分粒子集群（使用并查集算法）
     */
    void detectClusters();

    /**
     * @brief 为每个集群施加独立的向心力
     */
    void applyCohesionForces();

    /**
     * @brief 计算集群的质心和半径
     * @param cluster 集群引用
     */
    void computeClusterProperties(Cluster& cluster);

    /**
     * @brief 找到主集群（最大的集群）
     */
    void findMainCluster();
};

#endif // SLIME_CONTROLLER_H
