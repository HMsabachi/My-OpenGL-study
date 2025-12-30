#include "application.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include "../engine/engine.h"

Application* Application::mInstance = nullptr;

Application* Application::getInstance() { // Singleton
	if(mInstance == nullptr){
		mInstance = new Application();
	}
	return mInstance;
}

Application::Application() {

}

Application::~Application() {
	delete mInstance;
}

bool Application::init(const int& width, const int& height) {
	// initialize GLFW
	mWidth = width;mHeight = height;
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);// specify OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);// specify OpenGL version
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	mWindow = glfwCreateWindow(mWidth, mHeight, "LearnOpenGL", nullptr, nullptr); // create window
	if(mWindow == nullptr) return false; // check if window creation was successful
	glfwMakeContextCurrent(mWindow); //make the window context currently active
	glfwSwapInterval(0); // enable VSYNC


	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { // initialize GLAD
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return false;
	}
	// 设置窗口大小回调函数
	glfwSetFramebufferSizeCallback(mWindow, frameBufferSizeCallback);

	// 设置窗口用户指针为当前应用程序实例
	glfwSetWindowUserPointer(mWindow, this);

	// 设置键盘输入回调函数
	glfwSetKeyCallback(mWindow, keyCallback);

	// 设置鼠标按钮输入回调函数
	glfwSetMouseButtonCallback(mWindow, mouseButtonCallback);

	// 设置滚轮输入回调函数
	//glfwSetScrollCallback(mWindow, scrollCallback);


	return true; // initialization was successful
}

void Application::setMouse(int x, int y)
{
	mMouseX = x;
	mMouseY = y;
	glfwSetCursorPos(mWindow, x, y);
	this->getMouseMoveDistance();
}

void Application::updateDeltaTime() {
	currentTime = glfwGetTime();
	deltaTime = currentTime - lastFrameTime;
	
	// ✅ 修复：限制 deltaTime 上限，防止窗口暂停时的异常值
	const double maxDeltaTime = 0.1;  // 最大时间步长：100ms（相当于 10 FPS）
	if (deltaTime > maxDeltaTime) {
		deltaTime = maxDeltaTime;
	}
	
	// ✅ 防止负数或零（可能在时间倒退或第一帧时发生）
	if (deltaTime <= 0.0) {
		deltaTime = 0.016;  // 默认 60 FPS
	}
	
	lastFrameTime = currentTime;
	
	if (currentTime - fpsRecordTime >= 0.5f) {
		fpsRecordTime = currentTime;
		fps = (uint32_t)(1.0f / deltaTime);
	}
}
void Application::updateWindowTitle() {
	std::string title = "LearnOpenGL - FPS: " + std::to_string(fps) + "  - dt: " + std::to_string(deltaTime);
	glfwSetWindowTitle(mWindow, title.c_str());
}
void Application::updateStatus() { // 更新鼠标位置和窗口位置
	glfwGetCursorPos(mWindow, &mMouseX, &mMouseY);
	glfwGetWindowPos(mWindow, &mWindowPosX, &mWindowPosY);
}

bool Application::update() {
	if (glfwWindowShouldClose(mWindow)) return false; // 检查窗口是否应该关闭
	// 更新应用程序状态
	updateDeltaTime();
	updateStatus();
	updateWindowTitle();
	// 更新应用程序逻辑
	glfwPollEvents();
	// 调用用户定义的更新函数
	if(_updateFunction != nullptr)
		_updateFunction();

	// 反转缓冲区
	glfwSwapBuffers(mWindow);
	return true; 
}

void Application::destroy() {
	glfwTerminate(); // 终止 GLFW
}

void Application::frameBufferSizeCallback(GLFWwindow* window, int width, int height) {
	// 设置窗口大小回调函数
	Application* self = (Application*) glfwGetWindowUserPointer(window);
	self->mHeight = height;
	self->mWidth = width;
	if(self->mResizeCallback != nullptr){
		self->mResizeCallback(width, height);
	}
}
void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) { 
	// 设置键盘输入回调函数

	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	
	Application* self = (Application*)glfwGetWindowUserPointer(window);
	if(self->mKeyboardCallback != nullptr)
		self->mKeyboardCallback(key, action, mods);
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	Application* self = (Application*)glfwGetWindowUserPointer(window);
	if(self->mMouseButtonCallback != nullptr)
		self->mMouseButtonCallback(button, action, mods);
}
