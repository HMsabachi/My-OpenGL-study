#include "texture.h"
#include "../wrapper/checkError.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../application/stb_image.h"

Texture::Texture(const std::string& path, unsigned int unit) {
    // stbImage load picture
    int channels;

    //flip image on y axis
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(path.c_str(), &mWidth, &mHeight, &channels, STBI_rgb_alpha);

    // create texture
    GL_CALL(glGenTextures(1, &mTextureID));
    // active texture
    GL_CALL(glActiveTexture(GL_TEXTURE0 + unit));
    // bind texture
    GL_CALL(glBindTexture(GL_TEXTURE_2D, mTextureID));
    // transmit data to texture
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    // open mipmap
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    // release data
    stbi_image_free(data);

    mUnit = unit;
    //GL_CALL(glUniform1i(unit, mUnit));
}


Texture::~Texture() {
    if (mTextureID != 0) {
        GL_CALL(glDeleteTextures(1, &mTextureID));
    }
}

void Texture::use(int unit){
    mUnit = unit;
    GL_CALL(glActiveTexture(GL_TEXTURE0 + unit));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, mTextureID));
    
}
void Texture::use() const {
    GL_CALL(glActiveTexture(GL_TEXTURE0 + mUnit));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, mTextureID));
}

// texture_manager.cpp
#include <iostream>  // 用于错误输出

TextureManager::TextureManager() {}

TextureManager::~TextureManager() {
    releaseAll();
}

GLuint TextureManager::loadTexture(const std::string& filename, const std::string& name) {
    std::string texName = name.empty() ? filename : name;

    // 检查是否已加载
    auto it = m_textures.find(texName);
    if (it != m_textures.end()) {
        return it->second;
    }

    // 加载新纹理
    GLuint texture = loadTextureInternal(filename);
    if (texture != 0) {
        m_textures[texName] = texture;
    }
    return texture;
}

GLuint TextureManager::getTexture(const std::string& name) const {
    auto it = m_textures.find(name);
    return (it != m_textures.end()) ? it->second : 0;
}

bool TextureManager::releaseTexture(const std::string& name) {
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        glDeleteTextures(1, &it->second);
        m_textures.erase(it);
        return true;
    }
    return false;
}

void TextureManager::releaseAll() {
    for (auto& pair : m_textures) {
        glDeleteTextures(1, &pair.second);
    }
    m_textures.clear();
}

GLuint TextureManager::loadTextureInternal(const std::string& filename) {
    stbi_set_flip_vertically_on_load(true);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum internalFormat = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    GLenum dataFormat = (nrChannels == 4) ? GL_RGBA : GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

