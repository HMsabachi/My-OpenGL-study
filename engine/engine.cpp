#include "engine.h"
#include "../application/application.h"
#include <iostream>
#include <conio.h>
#include <string>
#include <memory>
#include <vector>

//#define DEBUG

#include "../wrapper/widgets.h"

#include "../glFrameWork/glFrameWork.h"
#include "../wrapper/checkError.h"


#include "camera.h"

#include "../application/stb_image.h" // 加载图片

#include "reactphysics3d/reactphysics3d.h" // 加载第三方物理引擎
#include "object/cube.h" // 引入Cube类
#include "object/sphere.h" // 引入Sphere类
#include "object/plane.h" // 引入Plane类
#include "object/slime.h" // 引入Slime类
#include "scene.h" // 引入Scene类

#define Ptr std::shared_ptr
#define MPtr std::make_shared


void setAcceleration(glm::vec3& acceleration)
{
    auto window = myApp->getWindow();
    if (glfwGetKey(window, GLFW_KEY_W))
        acceleration.z = -50.0f;
    if (glfwGetKey(window, GLFW_KEY_S))
        acceleration.z = 50.0f;
    if (glfwGetKey(window, GLFW_KEY_A))
        acceleration.x = -50.0f;
    if (glfwGetKey(window, GLFW_KEY_D))
        acceleration.x = 50.0f;
    if (glfwGetKey(window, GLFW_KEY_SPACE))
        acceleration.y = 50.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
        acceleration.y = -50.0f;
}

void Engine::update()
{
    float deltaTime = myApp->getDeltaTime();
    setAcceleration(this->cameraData.acceleration);
    this->updateCamera(deltaTime);

    if (glfwGetKey(myApp->getWindow(), GLFW_KEY_Z) == GLFW_PRESS)
    {
		camera->lookAt(glm::vec3(-2.0f, -5.0f, 0.0f), true);
	}

}

void Engine::updateCamera(float deltaTime)
{
    //std::cout << cameraData.acceleration.x << std::endl;
	// 计算相机位置更新
	cameraData.nowSpeed += cameraData.acceleration * deltaTime;
    cameraData.nowSpeed = glm::clamp(cameraData.nowSpeed, -cameraData.maxSpeed, cameraData.maxSpeed);
    glm::vec3 offset = cameraData.nowSpeed * deltaTime;
	//camera->translate(offset);
    camera->moveForward(-offset.z);
    camera->moveRight(offset.x);
    camera->moveUpFPS(offset.y);
	cameraData.acceleration = -cameraData.nowSpeed * 1.5f;
    cameraData.acceleration.y = -cameraData.nowSpeed.y * 6.0f;

	// 计算相机视角更新
    auto mOffset = myApp->getMouseMoveDistance();
    
    camera->processMouseMovement(mOffset.x, -mOffset.y);
    auto MousePos = myApp->getMousePos();
    
    if (mouseCaptured)
    {
        if (MousePos.x >= myApp->getWidth()-3) myApp->setMouse(6, MousePos.y);
        if (MousePos.x <= 3) myApp->setMouse(myApp->getWidth() - 6, MousePos.y);
        if (MousePos.y >= myApp->getHeight()-3) myApp->setMouse(MousePos.x, 6);
        if (MousePos.y <= 3) myApp->setMouse(MousePos.x, myApp->getHeight() - 6);
    }

    
}

int Engine::_initOpenGL()
{
	// 初始化窗口
	if(!myApp->init()) {
		std::cerr << "Failed to initialize application." << std::endl;
		return -1;
	}
	// 设置OpenGL状态
	GL_CALL(glEnable(GL_BLEND));
	GL_CALL(glEnable(GL_DEPTH_TEST));
	//glDisable(GL_FRAMEBUFFER_SRGB);
	GL_CALL(glViewport(0, 0, myApp->getWidth(), myApp->getHeight()));
	GL_CALL(glClearColor(0.2f, 0.3f, 0.3f, 1.0f));
	//myApp->setUpdateFunction(update);
	myApp->setResizeCallback(framebufferSizeCallback);

	return 0;
}

Engine::Engine() {
	
}

Engine::~Engine() {
    // 正确的资源释放顺序：
    
    // 1. 首先删除场景
    delete scene;
    scene = nullptr;
    
    // 2. 销毁物理世界
    if (pWorld) {
        physicsCommon.destroyPhysicsWorld(pWorld);
        pWorld = nullptr;
    }
    
    // 3. 删除 OpenGL 相关资源
    delete vao;
    vao = nullptr;
    
    delete shaderManager;
    shaderManager = nullptr;
    
    glDeleteTextures(1, &texture);
    glDeleteTextures(1, &texture2);
    
    // 4. 删除纹理管理器
    delete textureManager;
    textureManager = nullptr;
    
    // 5. 删除相机
    delete camera;
    camera = nullptr;
    
    // 6. 最后销毁 OpenGL 上下文
    myApp->destroy();
}

void Engine::setupDemoData()
{
    // 加载纹理
    texture = textureManager->loadTexture("assets/textures/container.jpg", "container");
    texture2 = textureManager->loadTexture("assets/textures/awesomeface.png", "awesomeface");
    
    // 配置 basic shader
    auto* basicShader = shaderManager->getShader("basic");
    basicShader->begin();
    basicShader->setInt("texture1", 0);  // 纹理单元 0
    basicShader->setInt("texture2", 1);  // 纹理单元 1
    basicShader->end();
    
    // 配置 sphere shader
    auto* sphereShader = shaderManager->getShader("sphere");
    sphereShader->begin();
    sphereShader->setInt("texture1", 0);  // 纹理单元 0
    sphereShader->end();

    // 设置相机
    camera->setFOV(60.0f);
    
    // 初始化全局 Uniform
    updateGlobalUniforms();
    
    // 创建地板
    Plane* floor = new Plane(this, glm::vec3(0.0f, -5.0f, 0.0f), glm::vec2(50.0f, 50.0f), basicShader, texture);
    floor->setTextureRepeat(10.0f, 10.0f);
    floor->initPhysics(Object::PhysicsType::STATIC, Object::CollisionShape::PLANE, glm::vec3(50.0f, 0.2f, 50.0f));
    scene->addObject(floor);
    
    // 创建 Cube 对象
    glm::vec3 cubePositions[] = {
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f,  2.0f, -2.5f),
        glm::vec3(1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    for (unsigned int i = 0; i < 10; i++)
    {
        Cube* cube = new Cube(this, cubePositions[i], glm::vec3(1.0f), basicShader, texture, texture2);
        float angle = 20.0f * i;
        cube->setRotation(angle, glm::vec3(1.0f, 0.3f, 0.5f));
        
        // 为部分 Cube 添加物理效果（前5个为动态物理对象）
        if (i < 5) {
            cube->initPhysics(Object::PhysicsType::DYNAMIC, Object::CollisionShape::BOX, glm::vec3(1.0f), 1.0f);
        }
        
        scene->addObject(cube);
    }

    // 创建 Sphere 对象
    Sphere* sphere = new Sphere(this, glm::vec3(3.0f, 0.0f, 0.0f), 1.0f, sphereShader, texture2);
    sphere->initPhysics(Object::PhysicsType::DYNAMIC, Object::CollisionShape::SPHERE, glm::vec3(1.0f), 1.0f);
    scene->addObject(sphere);

	// 创建 slime 对象
	// 优化参数：减少粒子数量，增大半径，提高流动性
	Slime* slime = new Slime(this, glm::vec3(-2.0f, -3.0f, 0.0f), 70, 0.08f, 1.5f, sphereShader, texture2);
    slime->setCohesionForce(0.1f);    // 适中的向心力
    slime->setDamping(0.98f);         // 适中的阻尼
    slime->setMaxCohesionDistance(1.5f); // 增大向心力作用距离
    scene->addObject(slime);
}

void Engine::updateGlobalUniforms() // 更新所有 Shader 的全局 Uniform
{
    glm::mat4 viewMatrix = camera->getViewMatrix();
    glm::mat4 projectionMatrix = camera->getProjectionMatrix();
    
    // 更新所有 Shader 的 View 和 Projection 矩阵
    auto* basicShader = shaderManager->getShader("basic");
    basicShader->begin();
    basicShader->setMat4("uView", viewMatrix);
    basicShader->setMat4("uProjection", projectionMatrix);
    basicShader->end();
    
    auto* sphereShader = shaderManager->getShader("sphere");
    sphereShader->begin();
    sphereShader->setMat4("uView", viewMatrix);
    sphereShader->setMat4("uProjection", projectionMatrix);
    sphereShader->end();
}

void Engine::render()
{
    while (myApp->update()) {
		this->update();
        
        // 每帧更新全局 Uniform
        updateGlobalUniforms();
        
        GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        
        // 更新并渲染场景
        float deltaTime = myApp->getDeltaTime();
        scene->update(deltaTime);
        scene->render();
        
        // 定期清理非活跃对象
        static float cleanupTimer = 0.0f;
        cleanupTimer += deltaTime;
        if (cleanupTimer >= 5.0f) {  // 每5秒清理一次
            scene->cleanupInactiveObjects();
            cleanupTimer = 0.0f;
        }
    }
}

void Engine::framebufferSizeCallback(int width, int height)
{
	// 调整视口大小
	Engine* self = myApp->engine;
	GL_CALL(glViewport(0, 0, width, height));
	self->camera->setAspect(static_cast<float>(width) / static_cast<float>(height));
	
	// 更新所有 Shader 的 Projection 矩阵
	self->updateGlobalUniforms();
}
void Engine::keyCallback(int key, int action, int mods)
{
    auto self = myApp->engine;
    auto window = myApp->getWindow();
    auto& cameraData = self->cameraData;
    
    if (action != GLFW_PRESS) return;
    switch (key)
    {
        case GLFW_KEY_LEFT_ALT:
            self->mouseCaptured = !self->mouseCaptured;
            glfwSetInputMode(window, GLFW_CURSOR, self->mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            break;
    }
    
}
int Engine::init() {
	myApp->engine = this;
    this->_initOpenGL();
    textureManager = new TextureManager();
    shaderManager = new ShaderManager();
    camera = new Camera(glm::vec3(-2.0f, -3.0f, 3.0f));
    camera->enableFPS(true);
    shaderManager->loadShader("basic", "assets/shaders/vertex.glsl", "assets/shaders/fragment.glsl");
    shaderManager->loadShader("sphere", "assets/shaders/sphere_vertex.glsl", "assets/shaders/sphere_fragment.glsl");

	myApp->setKeyboardCallback(keyCallback);

    // 初始化物理引擎
    this->pWorld = this->physicsCommon.createPhysicsWorld();
    
    // 设置重力
    this->pWorld->setGravity(rp3d::Vector3(0.0f, -9.81f, 0.0f));
    
    // 创建场景管理器
    scene = new Scene(this);

    return 0;
}