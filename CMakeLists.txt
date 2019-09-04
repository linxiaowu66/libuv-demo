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

set(HELLO_FILE
        ./src/hello_libuv.c)
set(IDLE_FILE
        ./src/idle.c)
set(FS_FILE
        ./src/fs.c)
add_executable(HelloUv ${HELLO_FILE})
add_executable(IdleHandle ${IDLE_FILE})
add_executable(FsHandle ${FS_FILE})