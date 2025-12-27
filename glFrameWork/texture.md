
### 使用文档

#### 概述
TextureManager类是为OpenGL C++渲染引擎设计的纹理管理类。它封装了纹理的加载、存储和释放操作，避免重复加载同一纹理，提高资源利用效率。依赖于stb_image.h库（用于图像加载）和GLAD（OpenGL函数）。该类使用std::unordered_map存储纹理ID，便于通过名称快速访问。

#### 安装和依赖
- 下载stb_image.h：https://github.com/nothings/stb
- 在项目中包含GLAD和stb_image.h。
- 在CMakeLists.txt或构建系统中链接OpenGL相关库。
- 如果已使用之前的texture_loader.h，可以直接整合，但本类内部实现了加载逻辑以独立运行。

#### 类成员和方法说明
- **构造函数**：初始化空管理器。示例：`TextureManager texManager;`
- **loadTexture**：加载纹理，支持自定义名称。如果名称已存在，返回现有ID。示例：`GLuint tex = texManager.loadTexture("assets/textures/container.jpg", "container");`
- **getTexture**：通过名称获取ID。示例：`GLuint tex = texManager.getTexture("container");`
- **releaseTexture**：释放指定纹理。示例：`texManager.releaseTexture("container");`
- **releaseAll**：释放所有纹理，通常在析构时自动调用。
- **析构函数**：自动释放所有资源。

#### 示例用法
1. 在引擎初始化中创建管理器：
   ```cpp
   #include "texture_manager.h"

   TextureManager textureManager;
   ```

2. 加载和使用纹理：
   ```cpp
   GLuint containerTex = textureManager.loadTexture("assets/textures/container.jpg", "container");
   GLuint faceTex = textureManager.loadTexture("assets/textures/awesomeface.png", "face");

   if (containerTex == 0 || faceTex == 0) {
       // 处理加载失败
   }

   // 在渲染循环中绑定
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, textureManager.getTexture("container"));
   glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
   ```

3. 清理资源（可选，手动释放）：
   ```cpp
   textureManager.releaseTexture("container");
   // 或在程序结束时依赖析构函数
   ```

#### 注意事项
- 纹理名称必须唯一；如果使用文件名作为名称，确保路径唯一。
- 加载失败时返回0，并输出错误到stderr。
- 在多线程环境中，确保OpenGL上下文正确（纹理操作需在主线程）。
- 如果需要自定义纹理参数（如不同过滤模式），可以扩展loadTextureInternal函数。
- 与之前提供的Camera类兼容，可集成到引擎中统一管理资源。
