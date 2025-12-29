file(GLOB_RECURSE EG ./*.cpp)
add_library(engine ${EG})

# engine/object/sphere.cpp 的 glob
file(GLOB_RECURSE OBJ_SOURCES "object/*.cpp" "object/*.h")
set_source_files_properties(${OBJ_SOURCES} PROPERTIES HEADER_FILE_ONLY FALSE)
target_sources(engine PRIVATE ${OBJ_SOURCES})