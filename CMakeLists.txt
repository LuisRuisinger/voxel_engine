cmake_minimum_required(VERSION 3.26)
project(opengl_3d_engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -mavx2")

find_package(OpenGL REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/glfw-3.3.9)
add_subdirectory(${CMAKE_SOURCE_DIR}/assimp)

include_directories(${CMAKE_SOURCE_DIR}/assimp/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/glfw-3.3.9/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/glad-2/include)
include_directories(${CMAKE_SOURCE_DIR}/lib)

set(ENABLE_BOOST_WORKAROUND ON)
set(BUILD_STATIC_LIB ON)
set(BUILD_ASSIMP_TOOLS  ON)
set(ASSIMP_BUILD_STATIC_LIB ON)

add_executable(
        opengl_3d_engine main.cpp
        rendering/Shader.cpp
        util/stb_image.cpp
        camera/camera.cpp
        level/model/Mesh.cpp
        level/octree/Octree.cpp
        level/Platform.cpp
        level/chunk/Chunk.cpp
        level/model/Model.cpp
        level/model/Types/Voxel.cpp
        rendering/Renderer.cpp
        camera/Culling.cpp
        level/octree/GreedyMeshing.cpp
        level/octree/Memorypool.cpp
        level/octree/Memorypool.h
        globalstate.h
        util/tickable.h
        util/moveable.h
        util/updateable.h
        util/indices_generator.h
)

target_sources(opengl_3d_engine PRIVATE ${CMAKE_SOURCE_DIR}/lib/glad-2/src/glad.c)
target_link_libraries(opengl_3d_engine glfw assimp)