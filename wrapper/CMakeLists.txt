# 递归遍历文件夹下所有的cpp
file(GLOB_RECURSE WRAPPER ./*.cpp)

# 编译静态链接库
add_library(wrapper ${WRAPPER})