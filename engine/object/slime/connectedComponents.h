// connectedComponents.h
#ifndef CONNECTED_COMPONENTS_H
#define CONNECTED_COMPONENTS_H

#include <glm/glm.hpp>
#include <vector>
#include <queue>
#include <unordered_map>  // ✅ 添加缺失的头文件

class DensityField;

/**
 * @struct ComponentInfo
 * @brief 单个连通块的信息
 */
struct ComponentInfo {
    std::vector<glm::vec3> particlePositions;  // 该块包含的粒子位置
    glm::vec3 boundsMin;                       // 包围盒最小值
    glm::vec3 boundsMax;                       // 包围盒最大值
    glm::vec3 centerOfMass;                    // 质心
    
    void clear() {
        particlePositions.clear();
        boundsMin = glm::vec3(0.0f);
        boundsMax = glm::vec3(0.0f);
        centerOfMass = glm::vec3(0.0f);
    }
    
    size_t particleCount() const { return particlePositions.size(); }
    
    // 计算包围盒（带边距）
    void computeBounds(float margin = 1.5f) {
        if (particlePositions.empty()) return;
        
        boundsMin = particlePositions[0];
        boundsMax = particlePositions[0];
        
        for (const auto& pos : particlePositions) {
            boundsMin = glm::min(boundsMin, pos);
            boundsMax = glm::max(boundsMax, pos);
        }
        
        // 添加边距
        glm::vec3 marginVec(margin);
        boundsMin -= marginVec;
        boundsMax += marginVec;
    }
    
    // 计算质心
    void computeCenterOfMass() {
        if (particlePositions.empty()) {
            centerOfMass = glm::vec3(0.0f);
            return;
        }
        
        centerOfMass = glm::vec3(0.0f);
        for (const auto& pos : particlePositions) {
            centerOfMass += pos;
        }
        centerOfMass /= static_cast<float>(particlePositions.size());
    }
};

/**
 * @class ConnectedComponents
 * @brief 连通域分析器，用于将粒子分组为独立的史莱姆块
 * 
 * 使用空间哈希和 BFS/DFS 算法识别连通的粒子组
 */
class ConnectedComponents {
public:
    ConnectedComponents() = default;
    ~ConnectedComponents() = default;

    /**
     * @brief 从粒子位置分析连通域
     * @param positions 所有粒子位置
     * @param searchRadius 搜索半径（粒子在此距离内被视为连接）
     * @param minComponentSize 最小连通块大小（粒子数）
     * @return 所有连通块的信息
     */
    std::vector<ComponentInfo> analyzeComponents(
        const std::vector<glm::vec3>& positions,
        float searchRadius,
        int minComponentSize = 5
    );

private:
    /**
     * @brief 使用 BFS 查找从指定粒子开始的连通域
     * @param startIdx 起始粒子索引
     * @param positions 所有粒子位置
     * @param visited 访问标记数组
     * @param searchRadiusSq 搜索半径的平方
     * @param cellSize 空间哈希格子大小
     * @return 连通块中的粒子索引
     */
    std::vector<int> findComponent(
        int startIdx,
        const std::vector<glm::vec3>& positions,
        std::vector<bool>& visited,
        float searchRadiusSq,
        float cellSize
    );
    
    /**
     * @brief 空间哈希函数
     */
    int getHashKey(const glm::vec3& pos, float cellSize) const;
    
    /**
     * @brief 构建空间哈希表
     */
    void buildSpatialHash(
        const std::vector<glm::vec3>& positions,
        float cellSize
    );
    
    /**
     * @brief 获取指定位置周围的粒子候选
     */
    std::vector<int> getCandidates(const glm::vec3& pos, float cellSize) const;
    
    // 空间哈希表
    std::unordered_map<int, std::vector<int>> m_spatialHash;
};

#endif // CONNECTED_COMPONENTS_H
