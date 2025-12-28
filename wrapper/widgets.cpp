#include "widgets.h"

#include <vector>
#include <cmath>
#include <glm/glm.hpp>  // 如果你使用glm库（推荐），否则可以用float[3]替换vec3




// 生成球体顶点数据
// 参数：
//   radius: 球体半径
//   sectors: 经度分段数（longitude，推荐 >= 12）
//   stacks: 纬度分段数（latitude，推荐 >= 8）
// 返回：交错的顶点数据 (position xyz + normal xyz + texcoord st)，每个顶点8个float
//         可直接用于glVertexAttribPointer
std::vector<float> widgets::generateSphereVertices(float radius, int sectors, int stacks) {
    std::vector<float> vertices;

    const float PI = 3.14159265359f;
    const float PI_2 = PI * 2.0f;

    // 循环栈（从北极到南极）
    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = PI / 2.0f - i * PI / stacks;  // 从 +90° 到 -90°
        float xy = radius * std::cos(stackAngle);       // r * cos(phi)
        float y = radius * std::sin(stackAngle);        // r * sin(phi)

        // 循环扇区（经度）
        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * PI_2 / sectors;     // 从 0 到 360°

            float x = xy * std::cos(sectorAngle);       // r * cos(phi) * cos(theta)
            float z = xy * std::sin(sectorAngle);       // r * cos(phi) * sin(theta)

            // 位置
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // 法线（单位球体上位置即为法线）
            vertices.push_back(x / radius);
            vertices.push_back(y / radius);
            vertices.push_back(z / radius);

            // 纹理坐标 (s, t)
            float s = static_cast<float>(j) / sectors;
            float t = static_cast<float>(i) / stacks;
            vertices.push_back(s);
            vertices.push_back(t);
        }
    }

    return vertices;
}

// 生成索引（用于GL_ELEMENT_ARRAY_BUFFER，绘制三角形）
std::vector<unsigned int> widgets::generateSphereIndices(int sectors, int stacks) {
    std::vector<unsigned int> indices;

    for (int i = 0; i < stacks; ++i) {
        unsigned int k1 = i * (sectors + 1);
        unsigned int k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            // 上半部分（除北极）
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            // 下半部分（除南极）
            if (i != (stacks - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    return indices;
}