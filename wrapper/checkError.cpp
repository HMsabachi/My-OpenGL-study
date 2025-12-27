#include "checkError.h"
#include <glad/glad.h>
#include <iostream>
#include <assert.h>
#include <string>

void checkError() {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::string err = "";
        switch (error) {
        case GL_INVALID_ENUM: err = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE: err = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: err = "INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY: err = "OUT_OF_MEMORY"; break;
        default: err = "UNKNOWN_ERROR"; break;
        }
        std::cerr << "OpenGL Error: " << err << std::endl;
        //assert(false); // assert if error occurs
    }
}