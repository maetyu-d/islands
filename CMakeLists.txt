cmake_minimum_required(VERSION 3.16)

project(WireframeCubeGrid LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(wireframe_grid src/main.cpp)

if(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
    find_library(GLUT_FRAMEWORK GLUT REQUIRED)

    target_link_libraries(wireframe_grid PRIVATE
        ${COCOA_FRAMEWORK}
        ${OPENGL_FRAMEWORK}
        ${GLUT_FRAMEWORK}
    )
endif()
