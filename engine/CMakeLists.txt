file(GLOB_RECURSE EG ./*.cpp)
add_library(engine ${EG})

# 确保包含 scene.cpp 和 object 子目录中的所有源文件
file(GLOB_RECURSE OBJ_SOURCES "object/*.cpp")
set_source_files_properties(${OBJ_SOURCES} PROPERTIES HEADER_FILE_ONLY FALSE)

# 打印调试信息
message(STATUS "Engine sources: ${EG}")
message(STATUS "Object sources: ${OBJ_SOURCES}")