#include "shader.h"
#include "../wrapper/checkError.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

Shader& Shader::create(const char* vertexPath, const char* fragmentPath){
    this->~Shader(); // 如果存在旧的着色器程序，先销毁
    // 创建新的着色器程序
    new (this) Shader(vertexPath, fragmentPath);
    return *this;
}

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
	// 声明字符串用于存储着色器代码
	std::string vertexCode;
	std::string fragmentCode;
	// 声明文件输入流
	std::ifstream vShaderFile;
	std::ifstream fShaderFile;

	// 确保 ifstream 对象可以抛出异常
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try {
		// 1. 打开文件
		vShaderFile.open(vertexPath);
		fShaderFile.open(fragmentPath);
		
		// 2. 将文件输入流中的内容读取到字符串流
		std::stringstream vShaderStream, fShaderStream;
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();

		// 3. 关闭文件句柄
		vShaderFile.close();
		fShaderFile.close();

		// 4. 将字符串流转换为代码字符串
		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
	}
	catch (std::ifstream::failure& e) {
		std::cout << "错误：着色器文件读取失败：" << e.what() << std::endl;
	}

    // 将代码字符串转换为 const char*
    const char* vertexShader = vertexCode.c_str();
    const char* fragmentShader = fragmentCode.c_str();
    
    // 创建顶点着色器
    GLuint VS = glCreateShader(GL_VERTEX_SHADER);
    GLuint FS = glCreateShader(GL_FRAGMENT_SHADER);
    
    // 设置着色器源代码
    glShaderSource(VS, 1, &vertexShader, NULL);
    glShaderSource(FS, 1, &fragmentShader, NULL);
    
    // 编译着色器
    glCompileShader(VS);
    checkShaderErrors(VS, "COMPILE"); // 检查编译错误
    glCompileShader(FS);
    checkShaderErrors(FS, "COMPILE"); // 检查编译错误
    
    // 创建着色器程序
    mProgID = glCreateProgram();
    
    // 将着色器附加到程序
    glAttachShader(mProgID, VS);
    glAttachShader(mProgID, FS);
    
    // 链接程序并检查错误
    glLinkProgram(mProgID);
    checkShaderErrors(mProgID, "LINK"); // 检查链接错误
    
    // 删除着色器对象（已经链接到程序中，不再需要）
    GL_CALL(glDeleteShader(VS));
    GL_CALL(glDeleteShader(FS));
}

Shader::~Shader() { // 析构函数
    if (m_isBound) {
        glUseProgram(0); // 如果当前绑定了该着色器，先解绑
    }
    GL_CALL(glDeleteProgram(mProgID)); // 删除着色器程序
}

void Shader::begin() { // 激活着色器程序
	GL_CALL(glUseProgram(mProgID));
    m_isBound = true;
} 

GLuint Shader::getAttribLocation(const char* name) { // 获取属性变量位置
    return GL_CALL(glGetAttribLocation(mProgID, name));
}

void Shader::end() { // 停用着色器程序
	GL_CALL(glUseProgram(0));
    m_isBound = false;
} 

void Shader::checkShaderErrors(GLuint target, std::string type) {
    int success = 0;
    char infoLog[1024];
    if (type == "COMPILE") {
        glGetShaderiv(target, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(target, 1024, NULL, infoLog);
            std::cout << "错误：着色器编译失败\n" << infoLog << std::endl;
        }
    }else if (type == "LINK") {
        glGetProgramiv(target, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(target, 1024, NULL, infoLog);
            std::cout << "错误：着色器程序链接失败\n" << infoLog << std::endl;
        }
    }
    else {
        std::cout << "错误：检查着色器错误的类型参数错误" << std::endl;
    }
}

// ========================================================================
// Uniform 设置函数
// ========================================================================

void Shader::setFloat(const char* name, float value) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin(); // 如果未激活，先激活
    GL_CALL(glUniform1f(location, value));
}

void Shader::setInt(const char* name, int value) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniform1i(location, value));
}

void Shader::setVec2(const char* name, float x, float y) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniform2f(location, x, y));
}

void Shader::setVec3(const char* name, float x, float y, float z) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniform3f(location, x, y, z));
}

void Shader::setVec4(const char* name, float x, float y, float z, float w) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniform4f(location, x, y, z, w));
}

void Shader::setVec4(const char* name, const glm::vec4& value) {
    setVec4(name, value.x, value.y, value.z, value.w);
}

void Shader::setMat2(const char* name, const glm::mat2& mat) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(mat)));
}

void Shader::setMat3(const char* name, const glm::mat3& mat) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(mat)));
}

void Shader::setMat4(const char* name, const glm::mat4& mat) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat)));
}

void Shader::setMat4(const char* name, const float* mat) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniformMatrix4fv(location, 1, GL_FALSE, mat));
}

// ========================================================================
// 传统 OpenGL 命名风格的 Uniform 设置函数
// ========================================================================

void Shader::setUniform1i(const char* name, int value) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniform1i(location, value));
}

void Shader::setUniform2i(const char* name, int v0, int v1) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniform2i(location, v0, v1));
}

void Shader::setUniform3i(const char* name, int v0, int v1, int v2) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniform3i(location, v0, v1, v2));
}

void Shader::setUniform4i(const char* name, int v0, int v1, int v2, int v3) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniform4i(location, v0, v1, v2, v3));
}

void Shader::setUniform1f(const char* name, float value) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniform1f(location, value));
}

void Shader::setUniform2f(const char* name, float v0, float v1) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
	GL_CALL(glUniform2f(location, v0, v1));
}

void Shader::setUniform3f(const char* name, float v0, float v1, float v2) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniform3f(location, v0, v1, v2));
}

void Shader::setUniform4f(const char* name, float v0, float v1, float v2, float v3) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    if (!m_isBound) begin();
    GL_CALL(glUniform4f(location, v0, v1, v2, v3));
}