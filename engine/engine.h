#ifndef ENGINE_H
#define ENGINE_H

#include "../glFrameWork/glFrameWork.h"
#include "camera.h"
#include "../application/application.h"

#include "reactphysics3d/reactphysics3d.h"

class Camera;
class TextureManager;
class ShaderManager;
class VAO;
class Cube; // 前向声明
class Sphere; // 前向声明
class Plane; // 前向声明
class Scene; // 前向声明
class PlayerController; // 前向声明

class Engine {
public:
	rp3d::PhysicsCommon physicsCommon;
	rp3d::PhysicsWorld* pWorld{nullptr};

public:
	ShaderManager* shaderManager{nullptr};
	Scene* scene{nullptr};  // 场景管理器
	PlayerController* playerController{nullptr};  // 玩家控制器

public:
	bool mouseCaptured{ false };

public: // 相机控制
	struct CameraData {
		glm::vec3 nowSpeed{ 0.0f, 0.0f, 0.0f };
		glm::vec3 maxSpeed{ 5.0f , 5.0f, 5.0f };
		glm::vec3 acceleration{ 0.0f, 0.0f, 5.0f};
		float mouseSensitivity{ 0.1f };
	};
	CameraData cameraData;
	void updateCamera(float deltaTime);

private:
	int _initOpenGL();

public:
	/**
	* @brief 默认构造函数
	*/
	Engine();
	/**
	* @brief 析构函数
	*/
	~Engine();
	/**
	* @brief 初始化引擎
	* @return 状态值
	*/
	int init();

    void update();
    void render();
    void setupDemoData();
    
    /**
     * @brief 更新所有 Shader 的全局 Uniform（View、Projection 矩阵）
     */
    void updateGlobalUniforms();

public:
    // 保留用于兼容性
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    VAO* vao;

public:
	TextureManager* textureManager{nullptr};
	Camera* camera{nullptr};
    
    GLuint texture{0};
    GLuint texture2{0};

public:
	static void framebufferSizeCallback(int width, int height);
	static void keyCallback(int key, int action, int mods);
};

#endif // ENGINE_H