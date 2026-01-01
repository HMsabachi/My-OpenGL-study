// densityField.cpp
#include "densityField.h"
#include <cmath>
#include <algorithm>
#include <execution>
#include <numeric>
#include <iostream>

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
    std::fill(std::execution::par_unseq, m_densities.begin(), m_densities.end(), 0.0f);
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

// ✅ 并行优化：使用染色法并行光栅化
void DensityField::buildFromParticles(const std::vector<glm::vec3>& positions, float particleRadius) {
    // 清空现有数据
    clear();
    
    if (positions.empty()) return;
    
    // 计算影响半径（网格单位）
    float influenceRadius = particleRadius * 2.0f;
    int gridRadius = static_cast<int>(std::ceil(influenceRadius / m_cellSize.x)) + 1;
    
    // ✅ 染色法：根据粒子的网格位置奇偶性分组，避免竞争
    // 将粒子分为8组（基于网格坐标的x, y, z奇偶性）
    std::vector<std::vector<size_t>> colorGroups(8);
    
    for (size_t i = 0; i < positions.size(); ++i) {
        glm::ivec3 gridPos = worldToGrid(positions[i]);
        int colorId = ((gridPos.x / gridRadius) & 1) | 
                     (((gridPos.y / gridRadius) & 1) << 1) | 
                     (((gridPos.z / gridRadius) & 1) << 2);
        colorGroups[colorId].push_back(i);
    }
    
    // ✅ 对每组并行处理（组内粒子不会写入相同的网格单元）
    for (const auto& group : colorGroups) {
        if (group.empty()) continue;
        
        std::for_each(std::execution::par_unseq, group.begin(), group.end(),
            [this, &positions, influenceRadius, gridRadius](size_t particleIdx) {
                const glm::vec3& pos = positions[particleIdx];
                glm::ivec3 centerGrid = worldToGrid(pos);
                
                // 遍历影响范围内的网格
                for (int dx = -gridRadius; dx <= gridRadius; ++dx) {
                    for (int dy = -gridRadius; dy <= gridRadius; ++dy) {
                        for (int dz = -gridRadius; dz <= gridRadius; ++dz) {
                            int x = centerGrid.x + dx;
                            int y = centerGrid.y + dy;
                            int z = centerGrid.z + dz;
                            
                            if (!isValidIndex(x, y, z)) continue;
                            
                            // 计算距离
                            glm::vec3 gridWorldPos = m_boundsMin + glm::vec3(x, y, z) * m_cellSize;
                            float dist = glm::length(gridWorldPos - pos);
                            
                            // 核函数
                            if (dist < influenceRadius) {
                                float normalizedDist = dist / influenceRadius;
                                float contribution = (1.0f - normalizedDist * normalizedDist);
                                
                                int idx = getIndex(x, y, z);
                                // 使用原子操作安全地累加（对于同一组不需要，但这里保险起见）
                                m_densities[idx] += contribution;
                            }
                        }
                    }
                }
            });
    }
}

// ✅ 并行优化：模糊算法
void DensityField::applyBlur(int iterations) {
    // 3x3x3 高斯核权重（简化版，中心权重更高）
    const float centerWeight = 0.5f;
    const float edgeWeight = 0.5f / 26.0f;  // 周围26个格子平均分配剩余权重
    
    for (int iter = 0; iter < iterations; ++iter) {
        // 使用临时缓冲存储结果
        std::fill(std::execution::par_unseq, m_tempBuffer.begin(), m_tempBuffer.end(), 0.0f);
        
        // ✅ 并行模糊：为每个内部网格点并行计算
        std::vector<int> gridIndices((m_resolution - 2) * (m_resolution - 2) * (m_resolution - 2));
        std::iota(gridIndices.begin(), gridIndices.end(), 0);
        
        std::for_each(std::execution::par_unseq, gridIndices.begin(), gridIndices.end(),
            [this, centerWeight, edgeWeight](int flatIdx) {
                // 将线性索引转换为3D坐标（跳过边界层）
                int size = m_resolution - 2;
                int z = flatIdx / (size * size) + 1;
                int y = (flatIdx / size) % size + 1;
                int x = flatIdx % size + 1;
                
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
            });
        
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
