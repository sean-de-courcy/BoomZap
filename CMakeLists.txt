cmake_minimum_required(VERSION 3.16)
project(BoomZap_0.4)

set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/source")
set(SOURCES "${SRC_DIR}/BoomZap.cpp")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")

# Executable definition and properties
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "/usr/local/include")
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

# glad
set(GLAD_DIR "${LIB_DIR}/glad")
add_library("glad" "${GLAD_DIR}/src/glad.c")
target_include_directories("glad" PRIVATE "${GLAD_DIR}/include")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLAD_DIR}/include")
target_link_libraries(${PROJECT_NAME} "glad")

target_link_libraries(${PROJECT_NAME} GL glfw3 X11 pthread freetype "${CMAKE_DL_LIBS}")
