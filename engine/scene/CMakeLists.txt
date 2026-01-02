# Scene 子模块 CMakeLists.txt

# 收集所有源文件
set(SCENE_SOURCES
    Transform.cpp
    GameObject.cpp
    SceneManager.cpp
)

# 收集所有头文件
set(SCENE_HEADERS
    Component.h
    Transform.h
    GameObject.h
    SceneManager.h
    LightComponent.h
)

# 添加到父项目的源文件列表
set(SCENE_ALL_FILES ${SCENE_SOURCES} ${SCENE_HEADERS})

# 将文件传递给父目录
set(SCENE_FILES ${SCENE_ALL_FILES} PARENT_SCOPE)

# 打印信息
message(STATUS "Scene module files: ${SCENE_ALL_FILES}")
