// densityField.h
#ifndef DENSITY_FIELD_H
#define DENSITY_FIELD_H

#include <glm/glm.hpp>
#include <vector>

/**
 * @class DensityField
 * @brief 3D密度场，用于将粒子光栅化并生成平滑的等值面
 * 
 * 该类负责：
 * - 将粒子光栅化到3D网格
 * - 应用高斯模糊使表面光滑
 * - 提供密度查询接口供 Marching Cubes 使用
 */
class DensityField {
public:
    /**
     * @brief 构造函数
     * @param bounds 密度场的边界框（最小和最大坐标）
     * @param resolution 每个维度的分辨率
     */
    DensityField(const glm::vec3& boundsMin, const glm::vec3& boundsMax, int resolution);
    
    ~DensityField() = default;

    /**
     * @brief 从粒子列表重建密度场
     * @param positions 粒子位置列表
     * @param particleRadius 粒子半径（影响光栅化范围）
     */
    void buildFromParticles(const std::vector<glm::vec3>& positions, float particleRadius);

    /**
     * @brief 应用高斯模糊（3x3x3核）
     * @param iterations 模糊迭代次数
     */
    void applyBlur(int iterations = 1);

    /**
     * @brief 获取指定位置的密度值
     * @param position 世界空间位置
     * @return 密度值（如果超出边界返回0）
     */
    float getDensity(const glm::vec3& position) const;

    /**
     * @brief 获取网格索引处的密度值
     * @param x, y, z 网格索引
     * @return 密度值
     */
    float getDensityAt(int x, int y, int z) const;

    /**
     * @brief 获取网格分辨率
     */
    int getResolution() const { return m_resolution; }

    /**
     * @brief 获取边界
     */
    glm::vec3 getBoundsMin() const { return m_boundsMin; }
    glm::vec3 getBoundsMax() const { return m_boundsMax; }
    glm::vec3 getCellSize() const { return m_cellSize; }

    /**
     * @brief 清空密度场
     */
    void clear();

private:
    int m_resolution;              // 每个维度的分辨率
    glm::vec3 m_boundsMin;         // 边界最小值
    glm::vec3 m_boundsMax;         // 边界最大值
    glm::vec3 m_cellSize;          // 单个体素大小
    
    std::vector<float> m_densities;  // 密度数据（线性存储）
    std::vector<float> m_tempBuffer; // 临时缓冲（用于模糊）

    /**
     * @brief 将3D索引转换为1D索引
     */
    inline int getIndex(int x, int y, int z) const {
        return x + y * m_resolution + z * m_resolution * m_resolution;
    }

    /**
     * @brief 检查索引是否在有效范围内
     */
    inline bool isValidIndex(int x, int y, int z) const {
        return x >= 0 && x < m_resolution &&
               y >= 0 && y < m_resolution &&
               z >= 0 && z < m_resolution;
    }

    /**
     * @brief 将世界坐标转换为网格索引
     */
    glm::ivec3 worldToGrid(const glm::vec3& position) const;

    /**
     * @brief 光栅化单个粒子到密度场
     * @param position 粒子位置
     * @param radius 粒子影响半径
     * @param strength 密度强度
     */
    void rasterizeParticle(const glm::vec3& position, float radius, float strength = 1.0f);
};

#endif // DENSITY_FIELD_H
