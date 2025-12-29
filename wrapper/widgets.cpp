#include "widgets.h"

#include <vector>
#include <cmath>
#include <glm/glm.hpp>  
#include <glm/gtc/constants.hpp>

namespace widgets {

    // Force recompile comment
    SphereData createSphere(float radius, int sectors, int stacks) {
        SphereData data;
        float x, y, z, xy;                              // vertex position
        float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
        float s, t;                                     // vertex texCoord

        float sectorStep = 2 * glm::pi<float>() / sectors;
        float stackStep = glm::pi<float>() / stacks;
        float sectorAngle, stackAngle;

        for (int i = 0; i <= stacks; ++i) {
            stackAngle = glm::pi<float>() / 2 - i * stackStep;        // starting from pi/2 to -pi/2
            xy = radius * cosf(stackAngle);             // r * cos(u)
            z = radius * sinf(stackAngle);              // r * sin(u)

            // add (sectorCount+1) vertices per stack
            // the first and last vertices have same position and normal, but different tex coords
            for (int j = 0; j <= sectors; ++j) {
                sectorAngle = j * sectorStep;           // starting from 0 to 2pi

                // vertex position (x, y, z)
                x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
                y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
                
                // vertex tex coord (s, t) range between [0, 1]
                s = (float)j / sectors;
                t = (float)i / stacks;

                // 重新计算以匹配 Y-up
                float y_pos = radius * sinf(stackAngle);
                float xy_pos = radius * cosf(stackAngle);
                float x_pos = xy_pos * cosf(sectorAngle);
                float z_pos = xy_pos * sinf(sectorAngle);

                // normalized vertex normal (nx, ny, nz)
                nx = x_pos * lengthInv;
                ny = y_pos * lengthInv;
                nz = z_pos * lengthInv;

                // Position (3 floats)
                data.vertices.push_back(x_pos);
                data.vertices.push_back(y_pos);
                data.vertices.push_back(z_pos);

                // Normal (3 floats)
                data.vertices.push_back(nx);
                data.vertices.push_back(ny);
                data.vertices.push_back(nz);

                // Texture coordinates (2 floats)
                data.vertices.push_back(s);
                data.vertices.push_back(t);
            }
        }

        // generate CCW index list of sphere triangles
        int k1, k2;
        for (int i = 0; i < stacks; ++i) {
            k1 = i * (sectors + 1);     // beginning of current stack
            k2 = k1 + sectors + 1;      // beginning of next stack

            for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
                // 2 triangles per sector excluding first and last stacks
                // k1 => k2 => k1+1
                if (i != 0) {
                    data.indices.push_back(k1);
                    data.indices.push_back(k2);
                    data.indices.push_back(k1 + 1);
                }

                // k1+1 => k2 => k2+1
                if (i != (stacks - 1)) {
                    data.indices.push_back(k1 + 1);
                    data.indices.push_back(k2);
                    data.indices.push_back(k2 + 1);
                }
            }
        }

        return data;
    }

}



