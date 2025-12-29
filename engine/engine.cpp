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

    myApp->destroy();
}



void Engine::setupDemoData()
{
    auto* shader = shaderManager->getShader("basic");

    shader->setInt("texture1", 0);
    shader->setInt("texture2", 1);

    camera->setFOV(60.0f);

    shader->setMat4("uView", camera->getViewMatrix());
    shader->setMat4("uProjection", camera->getProjectionMatrix());

	
    texture = textureManager->loadTexture("assets/textures/container.jpg", "container");
    texture2 = textureManager->loadTexture("assets/textures/awesomeface.png", "awesomeface");
    
    // 创建Cube对象
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
        Cube* cube = new Cube(this, cubePositions[i], glm::vec3(1.0f), shader, texture, texture2);
        float angle = 20.0f * i;
        cube->setRotation(angle, glm::vec3(1.0f, 0.3f, 0.5f));
        cubes.push_back(cube);
    }
}

void Engine::render()
{
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


    while (myApp->update()) {
		this->update();
        GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        auto* shader = shaderManager->getShader("basic");
        shader->setMat4("uView", camera->getViewMatrix());
        
        // 渲染所有Cube对象
        for (auto* cube : cubes) {
            cube->render();
        }
    }
}

void Engine::framebufferSizeCallback(int width, int height)
{
	// 调整视口大小
	Engine* self = myApp->engine;
	GL_CALL(glViewport(0, 0, width, height));
	self->camera->setAspect(static_cast<float>(width) / static_cast<float>(height));
	self->shaderManager->getShader("basic")->setMat4("uProjection", self->camera->getProjectionMatrix());
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

	myApp->setKeyboardCallback(keyCallback);

    // 初始化物理引擎
    this->pWorld = this->physicsCommon.createPhysicsWorld();

    return 0;

}