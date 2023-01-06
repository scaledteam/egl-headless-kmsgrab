
cmake_minimum_required(VERSION 3.5)

project(egl_headless)

include_directories("/usr/include/libdrm")
add_executable(main main.c egl.c)
target_link_libraries(main GLEW GL EGL drm)
