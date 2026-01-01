// slimeController.cpp
#include "slimeController.h"
#include "slime.h"
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <iostream>

// ✅ 并查集（Union-Find）用于检测连通分量
class UnionFind {
public:
    UnionFind(int n) : parent(n), rank(n, 0) {
        std::iota(parent.begin(), parent.end(), 0);
    }

    int find(int x) {
        if (parent[x] != x) {
            parent[x] = find(parent[x]);  // 路径压缩
        }
        return parent[x];
    }

    void unite(int x, int y) {
        int rootX = find(x);
        int rootY = find(y);
        if (rootX != rootY) {
            // 按秩合并
            if (rank[rootX] < rank[rootY]) {
                parent[rootX] = rootY;
            } else if (rank[rootX] > rank[rootY]) {
                parent[rootY] = rootX;
            } else {
                parent[rootY] = rootX;
                rank[rootX]++;
            }
        }
    }

private:
    std::vector<int> parent;
    std::vector<int> rank;
};

SlimeController::SlimeController(Slime* slime)
    : m_slime(slime),
      m_mainClusterIndex(0),
      m_cohesionRange(2.0f),  // 默认凝聚范围
      m_minClusterSize(10)     // 最小集群大小
{
}

void SlimeController::update(float deltaTime) {
    // 1. 检测粒子集群
    detectClusters();

    // 2. 找到主集群
    findMainCluster();

    // 3. 为每个集群施加独立的向心力
    applyCohesionForces();

    // 4. 输出调试信息（可选）
    static float debugTimer = 0.0f;
    debugTimer += deltaTime;
    if (debugTimer >= 2.0f) {
        std::cout << "[SlimeController] 集群数量: " << m_clusters.size() 
                  << " | 主集群大小: " << (m_clusters.empty() ? 0 : m_clusters[m_mainClusterIndex].size()) << std::endl;
        debugTimer = 0.0f;
    }
}

void SlimeController::detectClusters() {
    m_clusters.clear();

    // 获取史莱姆的粒子数据（需要添加访问接口）
    const auto& particles = m_slime->getParticles();
    const auto& neighbors = m_slime->getNeighbors();
    int particleCount = particles.size();

    if (particleCount == 0) return;

    // ✅ 使用并查集检测连通分量
    UnionFind uf(particleCount);

    // 将所有在凝聚范围内的邻居合并到同一集群
    for (int i = 0; i < particleCount; ++i) {
        for (int j : neighbors[i]) {
            glm::vec3 diff = particles[i].position - particles[j].position;
            float dist = glm::length(diff);

            // 只有在凝聚范围内才算同一集群
            if (dist < m_cohesionRange) {
                uf.unite(i, j);
            }
        }
    }

    // ✅ 收集每个集群的粒子
    std::unordered_map<int, std::vector<int>> clusterMap;
    for (int i = 0; i < particleCount; ++i) {
        int root = uf.find(i);
        clusterMap[root].push_back(i);
    }

    // ✅ 转换为 Cluster 结构
    for (const auto& [root, indices] : clusterMap) {
        if (indices.size() >= static_cast<size_t>(m_minClusterSize)) {
            Cluster cluster;
            cluster.particleIndices = indices;
            computeClusterProperties(cluster);
            m_clusters.push_back(cluster);
        }
    }

    // 按集群大小排序（大 -> 小）
    std::sort(m_clusters.begin(), m_clusters.end(), 
              [](const Cluster& a, const Cluster& b) {
                  return a.size() > b.size();
              });
}

void SlimeController::applyCohesionForces() {
    if (m_clusters.empty()) return;

    const auto& particles = m_slime->getParticles();
    const float cohesionStrength = m_slime->getCohesionStrength();
    const float slimeRadius = m_slime->getSlimeRadius();

    // ✅ 为每个集群独立施加向心力
    for (const auto& cluster : m_clusters) {
        glm::vec3 targetCenter = cluster.center + glm::vec3(0.0f, cluster.radius * 0.3f, 0.0f);

        for (int idx : cluster.particleIndices) {
            auto& particle = m_slime->getParticleMutable(idx);
            glm::vec3 toTarget = targetCenter - particle.position;
            float dist = glm::length(toTarget);

            if (dist < 0.001f) continue;

            // 计算向心力（与原始实现类似，但基于集群）
            float distanceFromCenter = glm::length(particle.position - cluster.center);
            float heightFactor = (particle.position.y - cluster.center.y) / cluster.radius;
            heightFactor = glm::clamp(heightFactor, -1.0f, 1.0f);
            float verticalMultiplier = 1.0f + heightFactor * 1.5f;

            if (distanceFromCenter > cluster.radius * 0.5f) {
                float excessDist = distanceFromCenter - cluster.radius * 0.5f;
                float forceMagnitude = cohesionStrength * (excessDist / cluster.radius) * verticalMultiplier;
                forceMagnitude = glm::min(forceMagnitude, cohesionStrength * 3.0f);
                particle.force += glm::normalize(toTarget) * forceMagnitude;
            }

            // 向下挤压效果
            if (heightFactor < -0.2f) {
                glm::vec3 radialDir = particle.position - cluster.center;
                radialDir.y = 0;
                float radialLen = glm::length(radialDir);

                if (radialLen > 0.001f) {
                    radialDir /= radialLen;
                    float outwardForce = cohesionStrength * 0.5f * (-heightFactor - 0.2f);
                    particle.force += radialDir * outwardForce;
                }
            }
        }
    }
}

void SlimeController::computeClusterProperties(Cluster& cluster) {
    if (cluster.particleIndices.empty()) return;

    const auto& particles = m_slime->getParticles();

    // 计算质心
    glm::vec3 center(0.0f);
    for (int idx : cluster.particleIndices) {
        center += particles[idx].position;
    }
    center /= static_cast<float>(cluster.particleIndices.size());
    cluster.center = center;

    // 计算半径（最远粒子距离）
    float maxDist = 0.0f;
    for (int idx : cluster.particleIndices) {
        float dist = glm::length(particles[idx].position - center);
        maxDist = std::max(maxDist, dist);
    }
    cluster.radius = maxDist;
}

void SlimeController::findMainCluster() {
    if (m_clusters.empty()) {
        m_mainClusterIndex = -1;
        return;
    }

    // 已经按大小排序，第一个就是主集群
    m_mainClusterIndex = 0;
}

void SlimeController::applyForceToMainCluster(const glm::vec3& force) {
    if (m_clusters.empty() || m_mainClusterIndex < 0) return;

    const auto& mainCluster = m_clusters[m_mainClusterIndex];
    const float forcePerParticle = 1.0f / static_cast<float>(mainCluster.size());
    const glm::vec3 distributedForce = force * forcePerParticle;

    // 只对主集群的粒子施加力
    for (int idx : mainCluster.particleIndices) {
        auto& particle = m_slime->getParticleMutable(idx);
        particle.force += distributedForce;
    }
}

glm::vec3 SlimeController::getMainClusterCenter() const {
    if (m_clusters.empty() || m_mainClusterIndex < 0) {
        return glm::vec3(0.0f);
    }
    return m_clusters[m_mainClusterIndex].center;
}

int SlimeController::getMainClusterSize() const {
    if (m_clusters.empty() || m_mainClusterIndex < 0) {
        return 0;
    }
    return m_clusters[m_mainClusterIndex].size();
}
