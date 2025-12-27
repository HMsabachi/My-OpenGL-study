#include "shader.h"
#include "../wrapper/checkError.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

Shader& Shader::create(const char* vertexPath, const char* fragmentPath){
    this->~Shader(); // delete old shader if it exists
    // create new shader
    new (this) Shader(vertexPath, fragmentPath);
    return *this;

}

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
	// declare string to hold shader code
	std::string vertexCode;
	std::string fragmentCode;
	// declare input file streams
	std::ifstream vShaderFile;
	std::ifstream fShaderFile;

	// make sure ifstream objects can throw exceptions
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try {
		// 1. open files
		vShaderFile.open(vertexPath);
		fShaderFile.open(fragmentPath);
		
		// 2. input the string from the file input stream into a string stream
		std::stringstream vShaderStream, fShaderStream;
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();

		// 3. close file handlers
		vShaderFile.close();
		fShaderFile.close();

		// 4. convert the string stream into the code string
		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
	}
	catch (std::ifstream::failure& e) {
		std::cout << "ERROR: Shader file error:" << e.what() << std::endl;
	}

    // convert the code strings to const char*
    const char* vertexShader = vertexCode.c_str();
    const char* fragmentShader = fragmentCode.c_str();
    // create program and shaders
    // create vertex shader
    GLuint VS = glCreateShader(GL_VERTEX_SHADER);
    GLuint FS = glCreateShader(GL_FRAGMENT_SHADER);
    // set shader source code
    glShaderSource(VS, 1, &vertexShader, NULL);
    glShaderSource(FS, 1, &fragmentShader, NULL);
    // compile shaders
    glCompileShader(VS);
    checkShaderErrors(VS, "COMPILE"); // check for errors
    glCompileShader(FS);
    checkShaderErrors(FS, "COMPILE"); // check for errors
    // create program
    mProgID = glCreateProgram();
    // attach shader to program
    glAttachShader(mProgID, VS);
    glAttachShader(mProgID, FS);
    // link program and check for errors
    glLinkProgram(mProgID);
    checkShaderErrors(mProgID, "LINK"); // check for errors
    // delete shaders
    GL_CALL(glDeleteShader(VS));
    GL_CALL(glDeleteShader(FS));
}
Shader::~Shader() { // destructor
    GL_CALL(glDeleteProgram(mProgID));
}

void Shader::begin() {// start using the shader
	GL_CALL(glUseProgram(mProgID));
} 

GLuint Shader::getAttribLocation(const char* name) { // get attribute location
    return GL_CALL(glGetAttribLocation(mProgID, name));
}

void Shader::end() { // stop using the shader
	GL_CALL(glUseProgram(0));
} 


void Shader::checkShaderErrors(GLuint target, std::string type) {
    int success = 0;
    char infoLog[1024];
    if (type == "COMPILE") {
        glGetShaderiv(target, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(target, 1024, NULL, infoLog);
            std::cout << "ERROR: Shader compile error \n" << infoLog << std::endl;
        }
    }else if (type == "LINK") {
        glGetProgramiv(target, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(target, 1024, NULL, infoLog);
            std::cout << "ERROR: Program link error\n" << infoLog << std::endl;
        }
    }
    else {
        std::cout << "ERROR: Check shader errors Type is wrong" << std::endl;
    }
}


void Shader::setFloat(const char* name, float value) { // set a float uniform variable in the shader
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    begin();
    GL_CALL(glUniform1f(location, value));
    end();
}
void Shader::setInt(const char* name, int value) { // set an integer uniform variable in the shader
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    begin();
    GL_CALL(glUniform1i(location, value));
    end();
}
void Shader::setVec2(const char* name, float x, float y) { // set a 2D vector uniform variable in the shader
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    begin();
    GL_CALL(glUniform2f(location, x, y));
    end();
}
void Shader::setVec3(const char* name, float x, float y, float z){ // set a 3D vector uniform variable in the shader
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    begin();
    GL_CALL(glUniform3f(location, x, y, z));
    end();
}

void Shader::setMat4(const char* name, const glm::mat4& mat) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    //std::cout << "shaderID:" << mProgID << " " << name << " location: " << location << std::endl;
    begin();
    GL_CALL(glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat)));
    end();
}
void Shader::setMat4(const char* name, const float* mat) {
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    //std::cout << "shaderID:" << mProgID << " " << name << " location: " << location << std::endl;
    begin();
    GL_CALL(glUniformMatrix4fv(location, 1, GL_FALSE, mat));
    end();
}






// set uniform functions

void Shader::setUniform1i(const char* name, int value) { // set an integer uniform variable in the shader
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    begin();
    GL_CALL(glUniform1i(location, value));
    end();
}
void Shader::setUniform2i(const char* name, int v0, int v1) { // set a 2D integer uniform variable in the shader
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    begin();
    GL_CALL(glUniform2i(location, v0, v1));
    end();
}
void Shader::setUniform3i(const char* name, int v0, int v1, int v2){ // set a 3D integer uniform variable in the shader
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    begin();
    GL_CALL(glUniform3i(location, v0, v1, v2));
    end();
}
void Shader::setUniform4i(const char* name, int v0, int v1, int v2, int v3) { // set a 4D integer uniform variable in the shader
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    begin();
    GL_CALL(glUniform4i(location, v0, v1, v2, v3));
    end();
}
void Shader::setUniform1f(const char* name, float value) { // set a float uniform variable in the shader
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    begin();
    GL_CALL(glUniform1f(location, value));
    end();
}
void Shader::setUniform2f(const char* name, float v0, float v1) { // set a 2D float uniform variable in the shader
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
	begin();
	GL_CALL(glUniform2f(location, v0, v1));
	end();
}
void Shader::setUniform3f(const char* name, float v0, float v1, float v2) { // set a 3D float uniform variable in the shader
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    begin();
    GL_CALL(glUniform3f(location, v0, v1, v2));
    end();
}
void Shader::setUniform4f(const char* name, float v0, float v1, float v2, float v3){ // set a 4D float uniform variable in the shader
    GLint location = GL_CALL(glGetUniformLocation(mProgID, name));
    begin();
    GL_CALL(glUniform4f(location, v0, v1, v2, v3));
    end();
}