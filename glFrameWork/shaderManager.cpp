// ShaderManager.cpp
#include "ShaderManager.h"
#include <iostream>  // for error logging

ShaderManager::ShaderManager() {}

ShaderManager::~ShaderManager() {
    clear();
}

bool ShaderManager::loadShader(const std::string& name, const char* vertexPath, const char* fragmentPath) {
    if (m_shaders.find(name) != m_shaders.end()) {
        std::cerr << "Shader with name '" << name << "' already exists." << std::endl;
        return false;
    }

    std::unique_ptr<Shader> shader = std::make_unique<Shader>();
    try {
        shader->create(vertexPath, fragmentPath);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load shader '" << name << "': " << e.what() << std::endl;
        return false;
    }

    m_shaders[name] = std::move(shader);
    return true;
}

Shader* ShaderManager::getShader(const std::string& name) const {
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        return it->second.get();
    }
    return nullptr;
}

void ShaderManager::clear() {
    m_shaders.clear();
}