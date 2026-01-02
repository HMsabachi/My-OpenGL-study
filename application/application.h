#pragma once
#include <iostream>
#include <glm/glm.hpp>

class Engine; // forward declaration

#define myApp Application::getInstance()

class GLFWwindow; // forward declaration
// wrapper for application

using ResizeCallback = void(*)(int width, int height);
using KeyboardCallback = void(*)(int key, int action, int mods);
using MouseButtonCallback = void(*)(int button, int action, int mods);
using ScrollCallback = void(*)(double xoffset, double yoffset);


// Singleton class

class Application {
private: //新增私人成员
	void(*_updateFunction)() = nullptr; // 更新函数指针

public: //新增公有成员
	double getDeltaTime() const { return deltaTime; } // 获取帧间隔时间
	glm::vec2 getMousePos() const { return glm::vec2(mMouseX, mMouseY); } // 获取鼠标位置
	glm::vec2 getMouseMoveDistance()  { auto distance = glm::vec2(mMouseX - lastMousePos.x, mMouseY - lastMousePos.y); lastMousePos = glm::vec2(mMouseX, mMouseY); return distance; } // 获取鼠标移动距
public:
	glm::vec2 lastMousePos{ 0 }; // 上一次鼠标位置
	double currentTime = 0.0;
	double lastFrameTime = 0.0;
	double deltaTime = 0.0;
	double fpsRecordTime = 0.0;
	uint32_t fps = 0;

public:
	~Application();
	// static method to get the instance of the application
	static Application* getInstance();

	bool init(const int& width = 1920 , const int& height = 1280);

	bool update();

	void destroy();
	
	GLFWwindow* getWindow() const { return mWindow; }

	int getWindowPosX() const { return mWindowPosX; }
	int getWindowPosY() const { return mWindowPosY; }

	uint32_t getWidth() const { return mWidth; }
	uint32_t getHeight() const { return mHeight; }

	float getMouseX() const { return mMouseX; }
	float getMouseY() const { return mMouseY; }
	void setMouse(int x, int y);


	void setUpdateFunction(void(*fun)()) { _updateFunction = fun; }

	void setResizeCallback(ResizeCallback callback) { mResizeCallback = callback; }
	void setKeyboardCallback(KeyboardCallback callback) { mKeyboardCallback = callback; }
	void setMouseButtonCallback(MouseButtonCallback callback) { mMouseButtonCallback = callback; }

private:
	void updateDeltaTime();
	void updateWindowTitle();
	void updateStatus();

private:
	static void frameBufferSizeCallback(GLFWwindow* window, int width, int height);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

private:
	static Application* mInstance;

	double mMouseX{ 0.0 }; // mouse x position
	double mMouseY{ 0.0 }; // mouse y position

	int mWindowPosX{ 0 }; // window position x
	int mWindowPosY{ 0 }; // window position y

	uint32_t mWidth{ 0 }; // window width
	uint32_t mHeight{ 0 }; // window height
	GLFWwindow* mWindow{ nullptr }; // GLFW window pointer

	ResizeCallback mResizeCallback{ nullptr }; // resize callback function pointer
	KeyboardCallback mKeyboardCallback{ nullptr }; // keyboard callback function pointer
	MouseButtonCallback mMouseButtonCallback{ nullptr }; // mouse button callback function pointer


	Application();

public:
	Engine* engine{ nullptr }; // engine pointer
};

