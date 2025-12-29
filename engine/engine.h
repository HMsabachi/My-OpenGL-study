#ifndef ENGINE_H
#define ENGINE_H

#include "../glFrameWork/core.h"
#include "../glFrameWork/shader.h"
#include "../glFrameWork/texture.h"
#include "camera.h"
#include "../application/application.h"
#include "../glFrameWork/buffers.h"

#include "reactphysics3d/reactphysics3d.h" // 加载第三方物理引擎



class Engine {
public:
	rp3d::PhysicsCommon physicsCommon;
	rp3d::PhysicsWorld* pWorld{nullptr};

public:
	bool mouseCaptured{ false };

public:
	void update();

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

public:
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;
	VAO* vao{nullptr};
	GLuint texture, texture2;
	void setupDemoData();

	void render();

public:
	Shader* shader{nullptr};
	TextureManager* textureManager{nullptr};
	Camera* camera{nullptr};

public:
	static void framebufferSizeCallback(int width, int height);
	static void keyCallback(int key, int action, int mods);
};









#endif // ENGINE_H