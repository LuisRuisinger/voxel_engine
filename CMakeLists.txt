cmake_minimum_required(VERSION 3.26)
project(opengl_3d_engine)

set(PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR})
add_definitions(-DPROJECT_ROOT_DIR=\"${PROJECT_ROOT_DIR}\")

set(CMAKE_CXX_STANDARD 20)

# Default to Release build if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Set compiler flags based on the build type
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -mavx2 -DNDEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -g -DDEBUG")
endif()

find_package(OpenGL REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/glfw-3.3.9)
add_subdirectory(${CMAKE_SOURCE_DIR}/assimp)

include_directories(${CMAKE_SOURCE_DIR}/assimp/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/glfw-3.3.9/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/glad-2/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/imgui)
include_directories(${CMAKE_SOURCE_DIR}/lib)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/lib/imgui)
add_library(imgui
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_impl_opengl3.cpp
        ${IMGUI_DIR}/imgui_impl_glfw.cpp
)

target_include_directories(imgui PUBLIC ${IMGUI_DIR})

set(ENABLE_BOOST_WORKAROUND ON)
set(BUILD_STATIC_LIB ON)
set(BUILD_ASSIMP_TOOLS  ON)
set(ASSIMP_BUILD_STATIC_LIB ON)

add_executable(
        opengl_3d_engine
        main.cpp
        rendering/shader.cpp
        util/stb_image.cpp
        camera/camera.cpp
        level/model/mesh.cpp
        level/octree/octree.cpp
        level/octree/octree.h
        level/platform.cpp
        level/chunk/chunk.cpp
        level/model/model.cpp
        level/model/Types/voxel.cpp
        rendering/renderer.cpp
        camera/culling.cpp
        globalstate.h
        util/tickable.h
        util/moveable.h
        util/updateable.h
        util/indices_generator.h
        level/chunk/chunk_segment.cpp
        level/chunk/chunk_segment.h
        threading/thread_pool.h
        threading/thread_safe_queue.h
        level/octree/node.cpp
        level/octree/node_inline.h
        rendering/interface.cpp
        level/presenter.cpp
        level/presenter.h
        util/log.cpp
        util/log.h
        util/assert.h
        util/atomic_shared_ptr.h
        util/perlin_noise.hpp
        util/tagged_ptr.h
        util/binary_conversion.h
        core/memory/linear_allocator_threadsafe.h
        core/memory/memory.h
        core/memory/arena_allocator.h
        level/model/tile/meshes/tile.cpp
        level/model/tile/meshes/tile.h
        level/model/tile/explicit/dirt.cpp
        level/model/tile/explicit/cobblestone.cpp
        level/model/tile/explicit/stone.cpp
        level/model/tile/tile_atlas.cpp
        level/model/tile/tile_atlas.h
        core/io/key_mapping.h
        core/memory/arena_allocator.cpp
)

target_sources(opengl_3d_engine PRIVATE ${CMAKE_SOURCE_DIR}/lib/glad-2/src/glad.c)
target_link_libraries(opengl_3d_engine imgui glfw assimp)

if (APPLE)
    target_link_libraries(opengl_3d_engine "-framework Cocoa" "-framework IOKit" "-framework CoreFoundation")
endif()