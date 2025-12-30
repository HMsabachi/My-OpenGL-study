#pragma once

#include "core.h"
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

/**
 * @class Shader
 * @brief OpenGL着色器程序封装类。
 *
 * 负责加载、编译顶点和片段着色器，链接生成着色器程序。
 * 提供Uniform变量设置接口，支持多种数据类型。
 */
class Shader {
public:
    /**
     * @brief 获取着色器程序ID。
     * @return GLuint 程序ID。
     */
	GLuint getProgramID() const { return mProgID; }

public:
    /**
     * @brief 默认构造函数。
     */
	Shader() = default;

    /**
     * @brief 构造函数，直接加载并编译着色器。
     * @param vertexPath 顶点着色器文件路径。
     * @param fragmentPath 片段着色器文件路径。
     */
	Shader(const char* vertexPath, const char* fragmentPath);

    /**
     * @brief 析构函数，销毁着色器程序。
     */
	~Shader();

    /**
     * @brief 创建着色器程序（重新加载）。
     * @param vertexPath 顶点着色器文件路径。
     * @param fragmentPath 片段着色器文件路径。
     * @return Shader& 返回自身引用。
     */
	Shader& create(const char* vertexPath, const char* fragmentPath);

    /**
     * @brief 激活着色器程序 (glUseProgram)。
     */
	void begin(); // start using the shader

    /**
     * @brief 获取属性变量的位置。
     * @param name 属性名称。
     * @return GLuint 属性位置。
     */
	GLuint getAttribLocation(const char* name); // get the location of an attribute variable in the shader

    /**
     * @brief 停用着色器程序。
     */
	void end(); // stop using the shader

private:
	// shader program
    /**
     * @brief 检查着色器编译或链接错误。
     * @param target 着色器或程序ID。
     * @param type 检查类型 ("COMPILE" 或 "LINK")。
     */
	void checkShaderErrors(GLuint target, std::string type); // check for errors in the shader compilation process and linking process

private:
	GLuint mProgID{ 0 };
    bool m_isBound{ false };  // 跟踪 Shader 是否已激活

public:
    /**
     * @brief 设置 float 类型 Uniform 变量。
     */
	void setFloat(const char* name, float value); // set a float uniform variable in the shader
    /**
     * @brief 设置 int 类型 Uniform 变量。
     */
	void setInt(const char* name, int value); // set an integer uniform variable in the shader
    /**
     * @brief 设置 vec2 类型 Uniform 变量。
     */
	void setVec2(const char* name, float x, float y); // set a 2D vector uniform variable in the shader
	void setVec2(const char* name, const glm::vec2& vec) { setVec2(name, vec.x, vec.y); }
    /**
     * @brief 设置 vec3 类型 Uniform 变量。
     */
	void setVec3(const char* name, float x, float y, float z); // set a 3D vector uniform variable in the shader

    /**
     * @brief 设置 mat4 类型 Uniform 变量。
     */
	void setMat4(const char* name, const glm::mat4& mat); // set a 4x4 matrix uniform variable in the shader
	void setMat4(const char* name, const float* mat); // set a 4x4 matrix uniform variable in the shader
    
    // ------------------------------------------------------------------------
    // 通用 set 函数重载，模拟自动类型推导
    // ------------------------------------------------------------------------
    /**
     * @brief 通用 Uniform 设置函数 (bool -> int)。
     */
    void set(const char* name, bool value) { setInt(name, (int)value); }
    /**
     * @brief 通用 Uniform 设置函数 (int)。
     */
    void set(const char* name, int value) { setInt(name, value); }
    /**
     * @brief 通用 Uniform 设置函数 (float)。
     */
    void set(const char* name, float value) { setFloat(name, value); }
    /**
     * @brief 通用 Uniform 设置函数 (vec2)。
     */
    void set(const char* name, const glm::vec2& value) { setVec2(name, value); }
    /**
     * @brief 通用 Uniform 设置函数 (vec3)。
     */
    void set(const char* name, const glm::vec3& value) { setVec3(name, value.x, value.y, value.z); }
    /**
     * @brief 通用 Uniform 设置函数 (vec4)。
     */
    void set(const char* name, const glm::vec4& value) { setVec4(name, value); }
    /**
     * @brief 通用 Uniform 设置函数 (mat2)。
     */
    void set(const char* name, const glm::mat2& value) { setMat2(name, value); }
    /**
     * @brief 通用 Uniform 设置函数 (mat3)。
     */
    void set(const char* name, const glm::mat3& value) { setMat3(name, value); }
    /**
     * @brief 通用 Uniform 设置函数 (mat4)。
     */
    void set(const char* name, const glm::mat4& value) { setMat4(name, value); }

    // ------------------------------------------------------------------------
    // 补充的特定类型设置函数
    // ------------------------------------------------------------------------
    /**
     * @brief 设置 vec4 类型 Uniform 变量。
     */
    void setVec4(const char* name, float x, float y, float z, float w);
    void setVec4(const char* name, const glm::vec4& value);
    /**
     * @brief 设置 mat2 类型 Uniform 变量。
     */
    void setMat2(const char* name, const glm::mat2& mat);
    /**
     * @brief 设置 mat3 类型 Uniform 变量。
     */
    void setMat3(const char* name, const glm::mat3& mat);

public:
    // 传统 OpenGL 命名风格的设置函数
	void setUniform1i(const char* name, int value); // set an integer uniform variable in the shader
	void setUniform2i(const char* name, int v0, int v1); // set a 2D integer uniform variable in the shader
	void setUniform3i(const char* name, int v0, int v1, int v2); // set a 3D integer uniform variable in the shader
	void setUniform4i(const char* name, int v0, int v1, int v2, int v3); // set a 4D integer uniform variable in the shader

	void setUniform1f(const char* name, float value); // set a float uniform variable in the shader
	void setUniform2f(const char* name, float v0, float v1); // set a 2D float uniform variable in the shader
	void setUniform3f(const char* name, float v0, float v1, float v2); // set a 3D float uniform variable in the shader
	void setUniform4f(const char* name, float v0, float v1, float v2, float v3); // set a 4D float uniform variable in the shad
};

/**
 * @class ComputeShader
 * @brief OpenGL计算着色器程序封装类。
 *
 * 负责加载、编译计算着色器，创建计算着色器程序。
 * 提供dispatch调用和Uniform变量设置接口。
 */
class ComputeShader {
public:
    /**
     * @brief 获取着色器程序ID。
     * @return GLuint 程序ID。
     */
    GLuint getProgramID() const { return mProgID; }

public:
    /**
     * @brief 默认构造函数。
     */
    ComputeShader() = default;

    /**
     * @brief 构造函数，直接加载并编译计算着色器。
     * @param computePath 计算着色器文件路径。
     */
    ComputeShader(const char* computePath);

    /**
     * @brief 析构函数，销毁着色器程序。
     */
    ~ComputeShader();

    /**
     * @brief 创建计算着色器程序（重新加载）。
     * @param computePath 计算着色器文件路径。
     * @return ComputeShader& 返回自身引用。
     */
    ComputeShader& create(const char* computePath);

    /**
     * @brief 激活计算着色器程序。
     */
    void begin();

    /**
     * @brief 停用计算着色器程序。
     */
    void end();

    /**
     * @brief 分派计算着色器工作组。
     * @param numGroupsX X维度工作组数量。
     * @param numGroupsY Y维度工作组数量。
     * @param numGroupsZ Z维度工作组数量。
     */
    void dispatch(GLuint numGroupsX, GLuint numGroupsY = 1, GLuint numGroupsZ = 1);

    /**
     * @brief 等待计算着色器完成执行。
     * @param barriers 内存屏障标志位（默认为GL_ALL_BARRIER_BITS）。
     */
    void wait(GLbitfield barriers = GL_ALL_BARRIER_BITS);

private:
    /**
     * @brief 检查着色器编译或链接错误。
     * @param target 着色器或程序ID。
     * @param type 检查类型 ("COMPILE" 或 "LINK")。
     */
    void checkShaderErrors(GLuint target, std::string type);

private:
    GLuint mProgID{ 0 };
    bool m_isBound{ false };

public:
    // Uniform 设置函数
    /**
     * @brief 设置 float 类型 Uniform 变量。
     */
    void setFloat(const char* name, float value);
    /**
     * @brief 设置 int 类型 Uniform 变量。
     */
    void setInt(const char* name, int value);
    /**
     * @brief 设置 vec2 类型 Uniform 变量。
     */
    void setVec2(const char* name, float x, float y);
    void setVec2(const char* name, const glm::vec2& vec) { setVec2(name, vec.x, vec.y); }
    /**
     * @brief 设置 vec3 类型 Uniform 变量。
     */
    void setVec3(const char* name, float x, float y, float z);
    void setVec3(const char* name, const glm::vec3& vec) { setVec3(name, vec.x, vec.y, vec.z); }
    /**
     * @brief 设置 vec4 类型 Uniform 变量。
     */
    void setVec4(const char* name, float x, float y, float z, float w);
    void setVec4(const char* name, const glm::vec4& value);
    /**
     * @brief 设置 mat2 类型 Uniform 变量。
     */
    void setMat2(const char* name, const glm::mat2& mat);
    /**
     * @brief 设置 mat3 类型 Uniform 变量。
     */
    void setMat3(const char* name, const glm::mat3& mat);
    /**
     * @brief 设置 mat4 类型 Uniform 变量。
     */
    void setMat4(const char* name, const glm::mat4& mat);
    void setMat4(const char* name, const float* mat);

    // 通用 set 函数重载
    /**
     * @brief 通用 Uniform 设置函数 (bool -> int)。
     */
    void set(const char* name, bool value) { setInt(name, (int)value); }
    /**
     * @brief 通用 Uniform 设置函数 (int)。
     */
    void set(const char* name, int value) { setInt(name, value); }
    /**
     * @brief 通用 Uniform 设置函数 (float)。
     */
    void set(const char* name, float value) { setFloat(name, value); }
    /**
     * @brief 通用 Uniform 设置函数 (vec2)。
     */
    void set(const char* name, const glm::vec2& value) { setVec2(name, value); }
    /**
     * @brief 通用 Uniform 设置函数 (vec3)。
     */
    void set(const char* name, const glm::vec3& value) { setVec3(name, value); }
    /**
     * @brief 通用 Uniform 设置函数 (vec4)。
     */
    void set(const char* name, const glm::vec4& value) { setVec4(name, value); }
    /**
     * @brief 通用 Uniform 设置函数 (mat2)。
     */
    void set(const char* name, const glm::mat2& value) { setMat2(name, value); }
    /**
     * @brief 通用 Uniform 设置函数 (mat3)。
     */
    void set(const char* name, const glm::mat3& value) { setMat3(name, value); }
    /**
     * @brief 通用 Uniform 设置函数 (mat4)。
     */
    void set(const char* name, const glm::mat4& value) { setMat4(name, value); }
};