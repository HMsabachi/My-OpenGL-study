#ifndef WIDGET_H
#define WIDGET_H

#include <vector>
#include <cmath>
#include <glm/glm.hpp>  


namespace widgets {
    
    struct SphereData {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
    };

    /**
     * @brief 生成球体的顶点和索引数据
     * @param radius 球体半径
     * @param sectors 切片数（经度方向）
     * @param stacks 堆叠数（纬度方向）
     * @return SphereData 包含顶点数据和索引数据的结构体
     * 
     * 顶点数据格式: [x, y, z, u, v] (位置 + 纹理坐标)
     */
    SphereData createSphere(float radius, int sectors, int stacks);

};



#endif // !


