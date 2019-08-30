cmake_minimum_required(VERSION 3.2)
project(LibuvDemo)

include_directories(/Users/linxiaowu/Github/libuv/include)
include_directories(/Users/linxiaowu/Github/libuv)

link_directories(
        /Users/linxiaowu/Github/libuv/out/Debug
)
link_libraries(
        uv
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -pthread")

set(SOURCE_FILES
        ./src/hello_libuv.c)
add_executable(HelloUv ${SOURCE_FILES})