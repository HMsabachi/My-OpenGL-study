#pragma once
#include "core.h"
#include <string>

class Texture {
public:
    Texture(const std::string& path, unsigned int unit);
    ~Texture();

public:
    void use(int unit);
    void use() const;

private:
    GLuint mTextureID{ 0 };
    int mWidth{ 0 };
    int mHeight{ 0 };
    unsigned int mUnit{ 0 };

};


// texture_manager.h
#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H
#include <glad/glad.h>  // OpenGL函数加载器
#include <unordered_map> // 无序映射，用于存储纹理
/**
 * @class TextureManager
 * @brief 纹理管理类，用于OpenGL渲染引擎中加载、管理和获取纹理。
 *
 * 该类提供纹理的加载、存储和检索功能，支持通过名称或路径加载纹理。
 * 自动处理重复加载，避免资源浪费。支持释放纹理以清理资源。
 */
class TextureManager {
public:
    /**
     * @brief 构造函数，初始化纹理管理器。
     */
    TextureManager();

    /**
     * @brief 析构函数，释放所有纹理资源。
     */
    ~TextureManager();

    /**
     * @brief 加载纹理，如果已存在则返回现有ID。
     * @param filename 纹理文件路径。
     * @param name 可选的纹理名称，用于后续检索。如果为空，则使用文件名作为名称。
     * @return 生成或现有的纹理ID（GLuint），如果加载失败返回0。
     */
    GLuint loadTexture(const std::string& filename, const std::string& name = "");

    /**
     * @brief 根据名称获取纹理ID。
     * @param name 纹理名称。
     * @return 纹理ID，如果不存在返回0。
     */
    GLuint getTexture(const std::string& name) const;

    /**
     * @brief 释放指定纹理。
     * @param name 纹理名称。
     * @return 是否成功释放。
     */
    bool releaseTexture(const std::string& name);

    /**
     * @brief 释放所有纹理资源。
     */
    void releaseAll();

private:
    std::unordered_map<std::string, GLuint> m_textures;  // 纹理名称到ID的映射

    // 内部加载函数
    GLuint loadTextureInternal(const std::string& filename);
};

#endif // TEXTURE_MANAGER_H