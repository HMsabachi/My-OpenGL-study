#ifndef WIDGET_H
#define WIDGET_H

#include <vector>
#include <cmath>
#include <glm/glm.hpp>  


namespace widgets {
    struct Vec2 {
        Vec2(float x = 0.0, float y = 0.0) : x(x), y(y) {}
        float x, y;
    };

    struct Vec3 {
        Vec3(float x = 0.0, float y = 0.0, float z = 0.0) : x(x), y(y), z(z) {}
        float x, y, z;
        Vec3 normalize() const {
            float len = sqrt(x * x + y * y + z * z);
            assert(len != 0);
            return Vec3{ x / len, y / len, z / len };
        }
    };

    struct Vertex {
        Vec3 position;
        Vec3 normal;
        Vec2 texCoord;
    };

    std::vector<float> transformVertices(const std::vector<Vertex>&  vertices) {
        std::vector<float> result;
        for (const Vertex& vertex : vertices) {
            result.push_back(vertex.position.x);
            result.push_back(vertex.position.y);
            result.push_back(vertex.position.z);
            result.push_back(vertex.normal.x);
            result.push_back(vertex.normal.y);
            result.push_back(vertex.normal.z);
            result.push_back(vertex.texCoord.x);
            result.push_back(vertex.texCoord.y);

        }
        return result;
    }


	std::vector<float> generateSphereVertices(float radius = 1.0f, int sectors = 36, int stacks = 18);
	// 生成索引（用于GL_ELEMENT_ARRAY_BUFFER，绘制三角形）
	std::vector<unsigned int> generateSphereIndices(int sectors = 36, int stacks = 18);


    class Ball {
    public:
        Ball(float radius = 1.0f, int xSegNum = 180, int ySegNum = 180) :
            m_radius(radius), m_xSegNum(xSegNum), m_ySegNum(ySegNum) {
            generateData();
        }
        ~Ball() {};

    private:
        void generateData() {
            const float PI = static_cast<float>(3.141592653589793);
            const float HalfPI = float(PI / 2.0);
            float dYaw = 2.0f * PI / float(m_xSegNum);
            float dPitch = PI / float(m_ySegNum);
            for (int i = 0; i <= m_xSegNum; ++i) {
                float yaw = i * dYaw;
                float U = (float)i / (float)m_xSegNum;
                for (int j = 0; j <= m_ySegNum; ++j) {
                    float pitch = -HalfPI + j * dPitch;             // 为了正确计算UV坐标，维度从南极开始计算
                    float V = (float)j / (float)m_ySegNum;
                    float x = m_radius * cos(pitch) * cos(yaw);
                    float y = m_radius * cos(pitch) * sin(yaw);
                    float z = m_radius * sin(pitch);

                    Vertex vertex;
                    vertex.position = Vec3(x, y, z);
                    vertex.normal = vertex.position.normalize();   // 球体的法线就是从原点指向顶点的向量
                    vertex.texCoord = Vec2(U, V);                  // 纹理坐标可以直接用U和V

                    vertices.push_back(vertex);
                }
            }

            for (int i = 0; i < m_xSegNum; ++i) {
                for (int j = 0; j < m_ySegNum; ++j) {
                    // 经纬网格面片的左下三角
                    indices.push_back(i * (m_ySegNum + 1) + j);
                    indices.push_back((i + 1) * (m_ySegNum + 1) + j);
                    indices.push_back(i * (m_ySegNum + 1) + j + 1);
                    // 经纬网格面片的右上三角
                    indices.push_back(i * (m_ySegNum + 1) + j + 1);
                    indices.push_back((i + 1) * (m_ySegNum + 1) + j);
                    indices.push_back((i + 1) * (m_ySegNum + 1) + j + 1);
                }
            }
        }

    public:
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

    private:
        float m_radius;
        int m_xSegNum;
        int m_ySegNum;
    };

};



#endif // !


