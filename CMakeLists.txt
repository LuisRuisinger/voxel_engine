cmake_minimum_required(VERSION 3.26)
project(opengl_3d_engine)

set(PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR})
add_definitions(-DPROJECT_ROOT_DIR=\"${PROJECT_ROOT_DIR}\")

set(CMAKE_CXX_STANDARD 20)

find_package(GLM REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")
find_package(GLFW3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")

# Default to Release build if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Set compiler flags based on the build type
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -mavx2 -DNDEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -mavx2 -g -DDEBUG")
endif()

# Find OpenGL (you are already using this correctly)
find_package(OpenGL REQUIRED)

# Add GLFW subdirectory
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/glfw-3.3.9)
set(GLFW_BUILD_SHARED OFF CACHE BOOL "Build GLFW as a static library" FORCE)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/lib/glfw-3.3.9/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/glad-3/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/imgui)
include_directories(${CMAKE_SOURCE_DIR}/lib)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/lib/imgui)

add_library(glad STATIC ${CMAKE_SOURCE_DIR}/lib/glad-3/src/glad.c)
add_library(imgui
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_impl_opengl3.cpp
        ${IMGUI_DIR}/imgui_impl_glfw.cpp
)

target_include_directories(imgui PUBLIC ${IMGUI_DIR})

# Use file(GLOB_RECURSE ...) to collect all .cpp files in your source directories
file(GLOB_RECURSE SOURCES
        ${CMAKE_SOURCE_DIR}/core/*.cpp
        ${CMAKE_SOURCE_DIR}/util/*.cpp
        # Add any other directories where your source files are located
)

# Create the executable target
add_executable(opengl_3d_engine
        ${SOURCES}
        ${CMAKE_SOURCE_DIR}/main.cpp
)


# Link OpenGL (already required above)
target_link_libraries(opengl_3d_engine
        OpenGL::GL
        imgui
        glfw
        glad
)

# Handle special Apple-specific linking
if (APPLE)
    target_link_libraries(opengl_3d_engine
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreFoundation"
            "-framework OpenGL"
    )
endif()
