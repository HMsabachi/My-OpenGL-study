// densityField.cpp
#include "densityField.h"
#include <cmath>
#include <algorithm>
#include <execution>

DensityField::DensityField(const glm::vec3& boundsMin, const glm::vec3& boundsMax, int resolution)
    : m_resolution(resolution),
      m_boundsMin(boundsMin),
      m_boundsMax(boundsMax)
{
    m_cellSize = (m_boundsMax - m_boundsMin) / static_cast<float>(m_resolution - 1);
    
    int totalCells = m_resolution * m_resolution * m_resolution;
    m_densities.resize(totalCells, 0.0f);
    m_tempBuffer.resize(totalCells, 0.0f);
}

void DensityField::clear() {
    std::fill(m_densities.begin(), m_densities.end(), 0.0f);
}

glm::ivec3 DensityField::worldToGrid(const glm::vec3& position) const {
    glm::vec3 normalized = (position - m_boundsMin) / (m_boundsMax - m_boundsMin);
    glm::ivec3 gridPos = glm::ivec3(normalized * static_cast<float>(m_resolution - 1));
    
    // 限制在有效范围内
    gridPos.x = glm::clamp(gridPos.x, 0, m_resolution - 1);
    gridPos.y = glm::clamp(gridPos.y, 0, m_resolution - 1);
    gridPos.z = glm::clamp(gridPos.z, 0, m_resolution - 1);
    
    return gridPos;
}

void DensityField::rasterizeParticle(const glm::vec3& position, float radius, float strength) {
    glm::ivec3 centerGrid = worldToGrid(position);
    
    // 计算粒子影响的网格范围
    int gridRadius = static_cast<int>(std::ceil(radius / m_cellSize.x)) + 1;
    
    for (int dx = -gridRadius; dx <= gridRadius; ++dx) {
        for (int dy = -gridRadius; dy <= gridRadius; ++dy) {
            for (int dz = -gridRadius; dz <= gridRadius; ++dz) {
                int x = centerGrid.x + dx;
                int y = centerGrid.y + dy;
                int z = centerGrid.z + dz;
                
                if (!isValidIndex(x, y, z)) continue;
                
                // 计算该网格点到粒子中心的距离
                glm::vec3 gridWorldPos = m_boundsMin + glm::vec3(x, y, z) * m_cellSize;
                float dist = glm::length(gridWorldPos - position);
                
                // 使用平滑核函数（反向二次函数）
                if (dist < radius) {
                    float normalizedDist = dist / radius;
                    float contribution = (1.0f - normalizedDist * normalizedDist) * strength;
                    
                    int idx = getIndex(x, y, z);
                    m_densities[idx] += contribution;
                }
            }
        }
    }
}

void DensityField::buildFromParticles(const std::vector<glm::vec3>& positions, float particleRadius) {
    // 清空现有数据
    clear();
    
    // 光栅化每个粒子
    for (const auto& pos : positions) {
        rasterizeParticle(pos, particleRadius * 2.0f, 1.0f);
    }
}

void DensityField::applyBlur(int iterations) {
    // 3x3x3 高斯核权重（简化版，中心权重更高）
    const float centerWeight = 0.5f;
    const float edgeWeight = 0.5f / 26.0f;  // 周围26个格子平均分配剩余权重
    
    for (int iter = 0; iter < iterations; ++iter) {
        // 使用临时缓冲存储结果
        std::fill(m_tempBuffer.begin(), m_tempBuffer.end(), 0.0f);
        
        // 并行模糊
        for (int z = 1; z < m_resolution - 1; ++z) {
            for (int y = 1; y < m_resolution - 1; ++y) {
                for (int x = 1; x < m_resolution - 1; ++x) {
                    float sum = 0.0f;
                    
                    // 当前格子
                    sum += m_densities[getIndex(x, y, z)] * centerWeight;
                    
                    // 周围26个格子
                    for (int dz = -1; dz <= 1; ++dz) {
                        for (int dy = -1; dy <= 1; ++dy) {
                            for (int dx = -1; dx <= 1; ++dx) {
                                if (dx == 0 && dy == 0 && dz == 0) continue;
                                sum += m_densities[getIndex(x + dx, y + dy, z + dz)] * edgeWeight;
                            }
                        }
                    }
                    
                    m_tempBuffer[getIndex(x, y, z)] = sum;
                }
            }
        }
        
        // 交换缓冲
        std::swap(m_densities, m_tempBuffer);
    }
}

float DensityField::getDensity(const glm::vec3& position) const {
    glm::ivec3 gridPos = worldToGrid(position);
    
    if (!isValidIndex(gridPos.x, gridPos.y, gridPos.z)) {
        return 0.0f;
    }
    
    return m_densities[getIndex(gridPos.x, gridPos.y, gridPos.z)];
}

float DensityField::getDensityAt(int x, int y, int z) const {
    if (!isValidIndex(x, y, z)) {
        return 0.0f;
    }
    return m_densities[getIndex(x, y, z)];
}
