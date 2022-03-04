cmake_minimum_required(VERSION 2.6)

project(agents)


find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLFW3 REQUIRED glfw3)
pkg_check_modules(GLEW REQUIRED glew)

pkg_check_modules(LIBAVCODEC REQUIRED libavcodec)
pkg_check_modules(LIBAVUTIL REQUIRED libavutil)

set(CMAKE_CXX_FLAGS "-O2 -Wall -Wextra")
SET(LIBRARIES glfw ${LIBAVCODEC_LDFLAGS} ${LIBAVUTIL_LDFLAGS} ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARY} ${LIBAVCODEC_LIBRARIES} ${LIBAVUTIL_LIBRARIES} ${CMAKE_DL_LIBS})

include_directories(${LIBAVCODEC_INCLUDE_DIRS} )
include_directories(${LIBAVUTIL_INCLUDE_DIRS})

ADD_LIBRARY(SlimeModule cava.cpp ffmpeg_decoder.cpp capture.cpp )
target_link_libraries(SlimeModule ${LIBAVCODEC_LDFLAGS})
target_link_libraries(SlimeModule ${LIBAVUTIL_LDFLAGS})
target_link_libraries(SlimeModule ${LIBRARIES})
target_link_libraries(SlimeModule ${LIBAVCODEC_LIBRARIES})
target_link_libraries(SlimeModule ${LIBAVUTIL_LIBRARIES})

ADD_EXECUTABLE(agents agents.cpp)
target_link_libraries(agents SlimeModule)
target_link_libraries(agents ${LIBRARIES})
message(${LIBAVCODEC_LIBRARIES})
message(${LIBAVCODEC_LDFLAGS})
message(${LIBAVCODEC_INCLUDE_DIRS})
