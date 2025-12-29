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
	delete textureManager;
	delete camera;
	//delete myApp;
    delete vao;
    delete shaderManager;

    glDeleteTextures(1, &texture);
    glDeleteTextures(1, &texture2);
    
    // 清理Cube对象
    for (auto* cube : cubes) {
        delete cube;
    }
    cubes.clear();

    // 清理Sphere对象
    for (auto* sphere : spheres) {
        delete sphere;
    }
    spheres.clear();

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
        cubes.push_back(cube);
    }

    // 创建 Sphere 对象
    Sphere* sphere = new Sphere(this, glm::vec3(3.0f, 0.0f, 0.0f), 1.0f, sphereShader, texture2);
    spheres.push_back(sphere);
}

void Engine::updateGlobalUniforms()
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
        
        // 渲染所有 Cube 对象
        for (auto* cube : cubes) {
            cube->render();
        }

        // 渲染所有 Sphere 对象
        for (auto* sphere : spheres) {
            sphere->render();
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
    camera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f));
    camera->enableFPS(true);
    shaderManager->loadShader("basic", "assets/shaders/vertex.glsl", "assets/shaders/fragment.glsl");
    shaderManager->loadShader("sphere", "assets/shaders/sphere_vertex.glsl", "assets/shaders/sphere_fragment.glsl");

	myApp->setKeyboardCallback(keyCallback);

    // 初始化物理引擎
    this->pWorld = this->physicsCommon.createPhysicsWorld();

    return 0;

}