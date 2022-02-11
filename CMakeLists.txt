cmake_minimum_required(VERSION 2.6)

project(agents)

find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLFW3 REQUIRED glfw3)
pkg_check_modules(GLEW REQUIRED glew)
pkg_check_modules(LIBPULSE-SIMPLE REQUIRED libpulse-simple)

set(CMAKE_CXX_FLAGS "-O2 -Wall -Wextra")
SET(LIBRARIES glfw ${GLFW_LIBRARIES} ${LIBPULSE-SIMPLE_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARY} ${CMAKE_DL_LIBS})

link_directories (${OPENGLEXAMPLES_BINARY_DIR}/bin)

add_executable (agents agents.cpp pulseAudio.cpp cava.cpp capture.cpp)
target_link_libraries(agents ${LIBRARIES} )
