cmake_minimum_required(VERSION 3.20)

project(threadpool)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS   "-g")                     # 调试信息
set(CMAKE_CXX_FLAGS   "-Wall")                  # 开启所有警告
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG " )   # release包优化

include_directories(include/)

set(
    SOURCES
    src/threadpool.cpp
    include/threadpool.hpp
    include/threadsafe_queue.hpp
)
add_library(
    threadpool
    ${SOURCES}
)

add_executable(
    example
    example/example.cpp
)
target_link_libraries(
    example
    threadpool
)