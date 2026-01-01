// marchingCubes.h
#ifndef MARCHING_CUBES_H
#define MARCHING_CUBES_H

#include <glm/glm.hpp>
#include <vector>

class DensityField;

/**
 * @struct MeshData
 * @brief 网格数据结构，包含顶点和索引
 */
struct MeshData {
    std::vector<glm::vec3> positions;  // 顶点位置
    std::vector<glm::vec3> normals;    // 顶点法线
    std::vector<unsigned int> indices; // 索引
    
    void clear() {
        positions.clear();
        normals.clear();
        indices.clear();
    }
    
    size_t vertexCount() const { return positions.size(); }
    size_t triangleCount() const { return indices.size() / 3; }
};

/**
 * @class MarchingCubes
 * @brief Marching Cubes 算法实现，将密度场转换为三角网格
 * 
 * 使用经典的 Marching Cubes 算法从3D密度场提取等值面
 */
class MarchingCubes {
public:
    MarchingCubes() = default;
    ~MarchingCubes() = default;

    /**
     * @brief 从密度场生成网格
     * @param densityField 输入密度场
     * @param isoLevel 等值面阈值（密度大于此值为实心）
     * @return 生成的网格数据
     */
    MeshData generateMesh(const DensityField& densityField, float isoLevel = 0.5f);

private:
    /**
     * @brief 处理单个立方体网格
     * @param densityField 密度场
     * @param x, y, z 立方体的网格坐标
     * @param isoLevel 等值面阈值
     * @param mesh 输出网格数据
     */
    void processCube(const DensityField& densityField, 
                     int x, int y, int z, 
                     float isoLevel, 
                     MeshData& mesh);

    /**
     * @brief 在两个顶点之间插值计算交点
     * @param p1, p2 两个顶点位置
     * @param v1, v2 两个顶点的密度值
     * @param isoLevel 等值面阈值
     * @return 插值后的位置
     */
    glm::vec3 interpolateVertex(const glm::vec3& p1, const glm::vec3& p2, 
                                float v1, float v2, 
                                float isoLevel);

    /**
     * @brief 计算顶点法线（使用密度梯度）
     * @param position 顶点位置
     * @param densityField 密度场
     * @return 法线向量
     */
    glm::vec3 calculateNormal(const glm::vec3& position, const DensityField& densityField);

    // Marching Cubes 查找表
    static const int edgeTable[256];
    static const int triTable[256][16];
};

#endif // MARCHING_CUBES_H
