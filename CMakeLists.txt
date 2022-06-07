cmake_minimum_required(VERSION 3.12)

project(qoipp CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PROJECT_SRC ${CMAKE_SOURCE_DIR}/src)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

add_library(qoipp INTERFACE)
target_include_directories(qoipp INTERFACE ${CMAKE_SOURCE_DIR}/include)

add_executable(test1 ${PROJECT_SRC}/../tests/test1.cpp)
target_link_libraries(test1 PRIVATE qoipp)
set_property(TARGET test1 PROPERTY CXX_STANDARD 20)


add_executable(test2 ${PROJECT_SRC}/../tests/test2.cpp)
set_property(TARGET test1 PROPERTY CXX_STANDARD 20)