#pragma once

#include "core.h"
#include <string>

class Shader {
public:
	GLuint getProgramID() const { return mProgID; }

public:
	Shader() = default;
	Shader(const char* vertexPath, const char* fragmentPath);
	~Shader();

	Shader& create(const char* vertexPath, const char* fragmentPath);

	void begin(); // start using the shader

	GLuint getAttribLocation(const char* name); // get the location of an attribute variable in the shader

	void end(); // stop using the shader

private:
	// shader program
	void checkShaderErrors(GLuint target, std::string type); // check for errors in the shader compilation process and linking process

private:
	GLuint mProgID{ 0 };

public:
	void setFloat(const char* name, float value); // set a float uniform variable in the shader
	void setInt(const char* name, int value); // set an integer uniform variable in the shader
	void setVec2(const char* name, float x, float y); // set a 2D vector uniform variable in the shader
	void setVec2(const char* name, const glm::vec2& vec) { setVec2(name, vec.x, vec.y); }
	void setVec3(const char* name, float x, float y, float z); // set a 3D vector uniform variable in the shader

	void setMat4(const char* name, const glm::mat4& mat); // set a 4x4 matrix uniform variable in the shader
	void setMat4(const char* name, const float* mat); // set a 4x4 matrix uniform variable in the shader
    

public:
	void setUniform1i(const char* name, int value); // set an integer uniform variable in the shader
	void setUniform2i(const char* name, int v0, int v1); // set a 2D integer uniform variable in the shader
	void setUniform3i(const char* name, int v0, int v1, int v2); // set a 3D integer uniform variable in the shader
	void setUniform4i(const char* name, int v0, int v1, int v2, int v3); // set a 4D integer uniform variable in the shader

	void setUniform1f(const char* name, float value); // set a float uniform variable in the shader
	void setUniform2f(const char* name, float v0, float v1); // set a 2D float uniform variable in the shader
	void setUniform3f(const char* name, float v0, float v1, float v2); // set a 3D float uniform variable in the shader
	void setUniform4f(const char* name, float v0, float v1, float v2, float v3); // set a 4D float uniform variable in the shad
};