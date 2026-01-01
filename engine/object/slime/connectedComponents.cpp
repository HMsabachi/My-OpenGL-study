// connectedComponents.cpp
#include "connectedComponents.h"
#include <cmath>
#include <algorithm>
#include <iostream>

std::vector<ComponentInfo> ConnectedComponents::analyzeComponents(
    const std::vector<glm::vec3>& positions,
    float searchRadius,
    int minComponentSize
) {
    std::vector<ComponentInfo> components;
    
    if (positions.empty()) {
        return components;
    }
    
    // 构建空间哈希表
    float cellSize = searchRadius;
    buildSpatialHash(positions, cellSize);
    
    // 访问标记
    std::vector<bool> visited(positions.size(), false);
    float searchRadiusSq = searchRadius * searchRadius;
    
    // 遍历所有粒子，查找连通域
    for (size_t i = 0; i < positions.size(); ++i) {
        if (visited[i]) continue;
        
        // 使用 BFS 查找连通块
        std::vector<int> componentIndices = findComponent(i, positions, visited, searchRadiusSq, cellSize);
        
        // 过滤太小的块
        if (componentIndices.size() < static_cast<size_t>(minComponentSize)) {
            continue;
        }
        
        // 创建连通块信息
        ComponentInfo info;
        info.particlePositions.reserve(componentIndices.size());
        
        for (int idx : componentIndices) {
            info.particlePositions.push_back(positions[idx]);
        }
        
        info.computeCenterOfMass();
        info.computeBounds(searchRadius * 1.5f);
        
        components.push_back(std::move(info));
    }
    
    //std::cout << "[ConnectedComponents] 检测到 " << components.size() 
              //<< " 个独立史莱姆块" << std::endl;
    
    return components;
}

std::vector<int> ConnectedComponents::findComponent(
    int startIdx,
    const std::vector<glm::vec3>& positions,
    std::vector<bool>& visited,
    float searchRadiusSq,
    float cellSize
) {
    std::vector<int> component;
    std::queue<int> queue;
    
    queue.push(startIdx);
    visited[startIdx] = true;
    
    while (!queue.empty()) {
        int currentIdx = queue.front();
        queue.pop();
        component.push_back(currentIdx);
        
        // 查找邻居
        const glm::vec3& currentPos = positions[currentIdx];
        std::vector<int> candidates = getCandidates(currentPos, cellSize);
        
        for (int neighborIdx : candidates) {
            if (visited[neighborIdx]) continue;
            
            // 检查距离
            glm::vec3 diff = positions[neighborIdx] - currentPos;
            float distSq = glm::dot(diff, diff);
            
            if (distSq <= searchRadiusSq) {
                visited[neighborIdx] = true;
                queue.push(neighborIdx);
            }
        }
    }
    
    return component;
}

void ConnectedComponents::buildSpatialHash(
    const std::vector<glm::vec3>& positions,
    float cellSize
) {
    m_spatialHash.clear();
    
    for (size_t i = 0; i < positions.size(); ++i) {
        int key = getHashKey(positions[i], cellSize);
        m_spatialHash[key].push_back(static_cast<int>(i));
    }
}

std::vector<int> ConnectedComponents::getCandidates(const glm::vec3& pos, float cellSize) const {
    std::vector<int> candidates;
    candidates.reserve(64);
    
    // 检查周围27个格子
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dz = -1; dz <= 1; ++dz) {
                glm::vec3 offset = glm::vec3(dx, dy, dz) * cellSize;
                int key = getHashKey(pos + offset, cellSize);
                
                auto it = m_spatialHash.find(key);
                if (it != m_spatialHash.end()) {
                    candidates.insert(candidates.end(), it->second.begin(), it->second.end());
                }
            }
        }
    }
    
    return candidates;
}

int ConnectedComponents::getHashKey(const glm::vec3& pos, float cellSize) const {
    int x = static_cast<int>(std::floor(pos.x / cellSize));
    int y = static_cast<int>(std::floor(pos.y / cellSize));
    int z = static_cast<int>(std::floor(pos.z / cellSize));
    
    return (x * 73856093) ^ (y * 19349663) ^ (z * 83492791);
}
