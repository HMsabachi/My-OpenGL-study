// ShaderManager.h
#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include "shader.h"  // 假设现有的Shader类
#include <string>
#include <unordered_map>
#include <memory>  // for std::unique_ptr

/**
 * @brief Shader管理类，用于加载、管理和检索多个Shader程序。
 * 该类使用unordered_map存储Shader实例，按名称访问。
 * 支持延迟加载或预加载，支持自动清理。
 */
class ShaderManager {
public:
    /**
     * @brief 默认构造函数。
     */
    ShaderManager();

    /**
     * @brief 析构函数，释放所有Shader资源。
     */
    ~ShaderManager();

    /**
     * @brief 加载一个Shader程序。
     * @param name Shader的唯一名称。
     * @param vertexPath 顶点着色器文件路径。
     * @param fragmentPath 片元着色器文件路径。
     * @return 是否加载成功。
     */
    bool loadShader(const std::string& name, const char* vertexPath, const char* fragmentPath);

    /**
     * @brief 获取指定名称的Shader指针。
     * @param name Shader名称。
     * @return Shader指针，如果不存在返回nullptr。
     */
    Shader* getShader(const std::string& name) const;

    /**
     * @brief 释放所有Shader资源。
     */
    void clear();

private:
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;  // Shader存储容器
};

#endif // SHADER_MANAGER_H