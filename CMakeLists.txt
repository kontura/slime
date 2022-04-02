cmake_minimum_required(VERSION 2.6)

project(agents)


find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLFW3 REQUIRED glfw3)
pkg_check_modules(GLEW REQUIRED glew)

pkg_check_modules(LIBAVCODEC REQUIRED libavcodec)
pkg_check_modules(LIBAVUTIL REQUIRED libavutil)
pkg_check_modules(LIBAVFORMAT REQUIRED libavformat)
pkg_check_modules(LIBSWSCALE REQUIRED libswscale)
pkg_check_modules(LIBSWRESAMPLE REQUIRED libswresample)
pkg_check_modules(FFTW3 REQUIRED fftw3)

set(CMAKE_CXX_FLAGS "-O2 -Wall -Wextra")
SET(LIBRARIES glfw ${LIBAVCODEC_LDFLAGS} ${LIBAVUTIL_LDFLAGS} ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARY} ${CMAKE_DL_LIBS})

include_directories(${LIBAVCODEC_INCLUDE_DIRS} )
include_directories(${LIBAVUTIL_INCLUDE_DIRS})
include_directories(${LIBAVFORMAT_INCLUDE_DIRS})
include_directories(${LIBSWSCALE_INCLUDE_DIRS})
include_directories(${LIBSWRESAMPLE_INCLUDE_DIRS})
include_directories(${FFTW3_INCLUDE_DIRS})

ADD_LIBRARY(SlimeModule cava.cpp ffmpeg_decoder.cpp)
target_link_libraries(SlimeModule ${LIBAVCODEC_LDFLAGS})
target_link_libraries(SlimeModule ${LIBAVUTIL_LDFLAGS})
target_link_libraries(SlimeModule ${LIBRARIES})
target_link_libraries(SlimeModule ${LIBAVCODEC_LIBRARIES})
target_link_libraries(SlimeModule ${LIBAVUTIL_LIBRARIES})
target_link_libraries(SlimeModule ${LIBAVFORMAT_LIBRARIES})
target_link_libraries(SlimeModule ${LIBSWSCALE_LIBRARIES})
target_link_libraries(SlimeModule ${LIBSWRESAMPLE_LIBRARIES})
target_link_libraries(SlimeModule ${FFTW3_LIBRARIES})

ADD_EXECUTABLE(agents agents.cpp)
target_link_libraries(agents SlimeModule)
target_link_libraries(agents ${LIBRARIES})
