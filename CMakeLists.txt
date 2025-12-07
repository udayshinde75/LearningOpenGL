cmake_minimum_required(VERSION 3.10)
project(OpenGLWindow)

set(CMAKE_CXX_STANDARD 20)

find_package(glfw3 REQUIRED)
find_library(OpenGL_LIBRARY OpenGL)

include_directories(glad/include)

add_executable(OpenGLWindow
        main.cpp
        glad/src/glad.c
)

target_link_libraries(OpenGLWindow
        glfw
        ${OpenGL_LIBRARY}
)
