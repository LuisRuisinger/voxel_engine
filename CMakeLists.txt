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
        Shader.cpp
        stb_image.cpp
        camera.cpp
        Level/Model/Mesh.cpp
        Level/Octree/Octree.cpp
        Level/Platform.cpp
        Level/Chunk/Chunk.cpp
        Level/Model/Model.cpp
        Level/Model/Types/Voxel.cpp
        Level/Quadtree/Quadtree.cpp
        Rendering/Renderer.cpp
        Rendering/Culling.cpp
        Level/Octree/GreedyMeshing.cpp
        Level/Octree/Memorypool.cpp
        Level/Octree/Memorypool.h
)

target_sources(opengl_3d_engine PRIVATE ${CMAKE_SOURCE_DIR}/lib/glad-2/src/glad.c)
target_link_libraries(opengl_3d_engine glfw assimp)