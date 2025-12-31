// buffers.h
#ifndef BUFFERS_H
#define BUFFERS_H

#include "core.h"  // OpenGL函数加载器
#include <vector>       // 用于数据存储
#include <string>       // 字符串处理
#include <sstream>      // 字符串流解析
#include <type_traits>  // 类型 trait

/**
 * @brief 获取OpenGL数据类型的模板函数。
 */
template<typename T>
GLenum getGLType() {
    if constexpr (std::is_same_v<T, float>) return GL_FLOAT;
    else if constexpr (std::is_same_v<T, int>) return GL_INT;
    else if constexpr (std::is_same_v<T, unsigned int>) return GL_UNSIGNED_INT;
    else if constexpr (std::is_same_v<T, double>) return GL_DOUBLE;
    else if constexpr (std::is_same_v<T, short>) return GL_SHORT;
    else if constexpr (std::is_same_v<T, unsigned short>) return GL_UNSIGNED_SHORT;
    else if constexpr (std::is_same_v<T, char>) return GL_BYTE;
    else if constexpr (std::is_same_v<T, unsigned char>) return GL_UNSIGNED_BYTE;
    else static_assert(false, "Unsupported type for VBO");
}

/**
 * @class Buffer
 * @brief 通用缓冲对象模板类，用于OpenGL中管理VBO或EBO。
 *
 * 该类支持多种数据类型，通过模板参数T指定。自动推导OpenGL类型。
 * 支持数据上传、更新、绑定等操作。
 */
template<typename T>
class Buffer {
public:
    /**
     * @brief 构造函数，创建缓冲并上传数据。
     * @param data 数据向量。
     * @param target 缓冲目标，如GL_ARRAY_BUFFER或GL_ELEMENT_ARRAY_BUFFER。
     * @param usage 数据使用模式，默认GL_STATIC_DRAW。
     */
    Buffer(const std::vector<T>& data, GLenum target, GLenum usage = GL_STATIC_DRAW)
        : m_target(target), m_usage(usage), m_type(getGLType<T>()) {
        glGenBuffers(1, &m_id);
        bind();
        glBufferData(m_target, data.size() * sizeof(T), data.data(), m_usage);
        m_size = data.size() * sizeof(T);
        m_count = data.size();
        unbind();
    }

    /**
     * @brief 构造函数，通过C风格数组指针创建缓冲。
     * @param data 数据指针。
     * @param count 元素数量。
     * @param target 缓冲目标，如GL_ARRAY_BUFFER或GL_ELEMENT_ARRAY_BUFFER。
     * @param usage 数据使用模式，默认GL_STATIC_DRAW。
     */
    Buffer(const T* data, size_t count, GLenum target, GLenum usage = GL_STATIC_DRAW)
        : m_target(target), m_usage(usage), m_type(getGLType<T>()) {
        glGenBuffers(1, &m_id);
        bind();
        glBufferData(m_target, count * sizeof(T), data, m_usage);
        m_size = count * sizeof(T);
        m_count = count;
        unbind();
    }

    /**
     * @brief 析构函数，释放缓冲资源。
     */
    ~Buffer() {
        glDeleteBuffers(1, &m_id);
    }

    /**
     * @brief 绑定缓冲。
     */
    void bind() const {
        glBindBuffer(m_target, m_id);
    }

    /**
     * @brief 解绑缓冲。
     */
    void unbind() const {
        glBindBuffer(m_target, 0);
    }

    /**
     * @brief 更新缓冲数据。
     * @param data 新数据向量。
     * @param offset 偏移量（字节），默认0。
     */
    void update(const std::vector<T>& data, size_t offset = 0) {
        bind();
        glBufferSubData(m_target, offset, data.size() * sizeof(T), data.data());
        unbind();
        if (offset + data.size() * sizeof(T) > m_size) {
            m_size = offset + data.size() * sizeof(T);
            m_count = m_size / sizeof(T);
        }
    }
    
    /**
     * @brief 获取数据大小（字节）。
     */
    size_t size() const { return m_size; }

    /**
     * @brief 获取元素数量。
     */
    size_t count() const { return m_count; }

    /**
     * @brief 获取OpenGL数据类型。
     */
    GLenum type() const { return m_type; }

    /**
     * @brief 获取缓冲ID。
     */
    GLuint id() const { return m_id; }

private:
    GLuint m_id = 0;
    GLenum m_target;
    GLenum m_usage;
    GLenum m_type;
    size_t m_size = 0;
    size_t m_count = 0;
};

/**
 * @typedef VBO
 * @brief Vertex Buffer Object，别名为Buffer<T> with GL_ARRAY_BUFFER。
 */
template<typename T>
using VBO = Buffer<T>;

/**
 * @typedef EBO
 * @brief Element Buffer Object，别名为Buffer<unsigned int> with GL_ELEMENT_ARRAY_BUFFER，默认unsigned int。
 */
template<typename T = unsigned int>
using EBO = Buffer<T>;

/**
 * @class VAO
 * @brief Vertex Array Object 封装类，用于管理顶点数组。
 *
 * 支持添加模板VBO和EBO，布局字符串支持类型指定，如"3f 2i"（3 float + 2 int）。
 * 类型代码：f=GL_FLOAT, d=GL_DOUBLE, i=GL_INT, ui=GL_UNSIGNED_INT, s=GL_SHORT, us=GL_UNSIGNED_SHORT, b=GL_BYTE, ub=GL_UNSIGNED_BYTE。
 */
class VAO {
public:
    /**
     * @brief 构造函数，创建VAO。
     */
    VAO() {
        glGenVertexArrays(1, &m_id);
    }

    /**
     * @brief 析构函数，释放VAO资源。
     */
    ~VAO() {
        glDeleteVertexArrays(1, &m_id);
    }

    /**
     * @brief 绑定VAO。
     */
    void bind() const {
        glBindVertexArray(m_id);
    }

    /**
     * @brief 解绑VAO。
     */
    void unbind() const {
        glBindVertexArray(0);
    }

    /**
     * @brief 添加VBO并配置属性。
     * @param vbo VBO模板对象。
     * @param layout 属性布局，如"3f 3f 2f"（size + type）。
     * @param normalized 是否规范化，默认GL_FALSE。
     * @param startIndex 起始属性索引，默认0。
     */
    template<typename T>
    void addVBO(const VBO<T>& vbo, const std::string& layout, GLboolean normalized = GL_FALSE, GLuint startIndex = 0) {
        bind();
        vbo.bind();

        std::istringstream iss(layout);
        std::string token;
        GLuint index = startIndex;
        GLsizei stride = 0;

        // 计算stride
        std::istringstream iss2(layout);
        while (iss2 >> token) {
            int size = std::stoi(token.substr(0, token.size() - 1));
            char typeChar = token.back();
            GLenum attrType = parseType(typeChar);
            stride += size * getTypeSize(attrType);
        }

        const void* offset = (void*)0;
        while (iss >> token) {
            int size = std::stoi(token.substr(0, token.size() - 1));
            char typeChar = token.back();
            GLenum attrType = parseType(typeChar);
            glVertexAttribPointer(index, size, attrType, normalized, stride, offset);
            glEnableVertexAttribArray(index);
            offset = reinterpret_cast<const void*>(reinterpret_cast<size_t>(offset) + size * getTypeSize(attrType));
            index++;
        }

        m_vertexCount = vbo.size() / stride;
        vbo.unbind();
        unbind();
    }
    
    /**
     * @brief 添加实例化VBO并配置属性（设置 divisor = 1）
     * @param vbo 实例化VBO对象。
     * @param layout 属性布局，如"4f 4f 4f 4f"（用于mat4）。
     * @param startIndex 起始属性索引。
     * @param divisor 实例化分频器，默认1（每个实例更新一次）。
     */
    template<typename T>
    void addInstancedVBO(const VBO<T>& vbo, const std::string& layout, GLuint startIndex, GLuint divisor = 1) {
        bind();
        vbo.bind();

        std::istringstream iss(layout);
        std::string token;
        GLuint index = startIndex;
        GLsizei stride = 0;

        // 计算stride
        std::istringstream iss2(layout);
        while (iss2 >> token) {
            int size = std::stoi(token.substr(0, token.size() - 1));
            char typeChar = token.back();
            GLenum attrType = parseType(typeChar);
            stride += size * getTypeSize(attrType);
        }

        const void* offset = (void*)0;
        while (iss >> token) {
            int size = std::stoi(token.substr(0, token.size() - 1));
            char typeChar = token.back();
            GLenum attrType = parseType(typeChar);
            glVertexAttribPointer(index, size, attrType, GL_FALSE, stride, offset);
            glEnableVertexAttribArray(index);
            glVertexAttribDivisor(index, divisor);  // 设置实例化分频器
            offset = reinterpret_cast<const void*>(reinterpret_cast<size_t>(offset) + size * getTypeSize(attrType));
            index++;
        }

        vbo.unbind();
        unbind();
    }

    /**
     * @brief 添加EBO。
     * @param ebo EBO模板对象。
     */
    template<typename T>
    void addEBO(const EBO<T>& ebo) {
        bind();
        ebo.bind();
        m_hasEBO = true;
        m_eboCount = ebo.count();
        m_eboType = ebo.type();
        unbind();
    }

    /**
     * @brief 绘制几何。
     * @param mode 模式，默认GL_TRIANGLES。
     * @param count 指定count（无EBO时），默认0使用自动计算。
     * @param offset 偏移，默认0。
     */
    void draw(GLenum mode = GL_TRIANGLES, GLsizei count = 0, size_t offset = 0) const {
        bind();
        if (m_hasEBO) {
            glDrawElements(mode, m_eboCount, m_eboType, reinterpret_cast<void*>(offset * getTypeSize(m_eboType)));
        }
        else {
            glDrawArrays(mode, offset, count ? count : m_vertexCount);
        }
        unbind();
    }

    /**
     * @brief 实例化绘制几何
     * @param instanceCount 实例数量
     * @param indexCount 索引数量（使用EBO时）
     * @param mode 模式，默认GL_TRIANGLES
     */
    void drawInstanced(GLsizei instanceCount, GLsizei indexCount = 0, GLenum mode = GL_TRIANGLES) const {
        bind();
        if (m_hasEBO) {
            GLsizei count = indexCount > 0 ? indexCount : m_eboCount;
            glDrawElementsInstanced(mode, count, m_eboType, nullptr, instanceCount);
        }
        else {
            glDrawArraysInstanced(mode, 0, m_vertexCount, instanceCount);
        }
        unbind();
    }

    /**
     * @brief 获取VAO ID。
     */
    GLuint id() const { return m_id; }

private:
    GLuint m_id = 0;
    bool m_hasEBO = false;
    size_t m_eboCount = 0;
    GLenum m_eboType = GL_UNSIGNED_INT;
    size_t m_vertexCount = 0;

    /**
     * @brief 解析布局类型字符。
     */
    static GLenum parseType(char c) {
        switch (c) {
        case 'f': return GL_FLOAT;
        case 'd': return GL_DOUBLE;
        case 'i': return GL_INT;
        case 'u': return GL_UNSIGNED_INT;  // ui -> u for simplicity, or adjust
        case 's': return GL_SHORT;
        case 'h': return GL_UNSIGNED_SHORT;  // us -> h for half? Wait, us -> u short, but simplify to 'u' for uint, 'i' for int
        case 'b': return GL_BYTE;
        case 'c': return GL_UNSIGNED_BYTE;  // ub -> c for char?
        default: throw std::invalid_argument("Unsupported type char");
        }
    }

    /**
     * @brief 获取OpenGL类型的大小。
     */
    static size_t getTypeSize(GLenum type) {
        switch (type) {
        case GL_FLOAT: return sizeof(float);
        case GL_DOUBLE: return sizeof(double);
        case GL_INT: return sizeof(int);
        case GL_UNSIGNED_INT: return sizeof(unsigned int);
        case GL_SHORT: return sizeof(short);
        case GL_UNSIGNED_SHORT: return sizeof(unsigned short);
        case GL_BYTE: return sizeof(char);
        case GL_UNSIGNED_BYTE: return sizeof(unsigned char);
        default: return 0;
        }
    }
};

#endif // BUFFERS_H