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
set(TCP_FILE
        ./src/tcpserver.c)
set(UDP_FILE
        ./src/udpserver.c)
set(PROCESS_FILE
        ./src/process.c)
set(THREAD_FILE
        ./src/thread.c)
set(DNS_FILE
        ./src/dns.c)
set(PIPE_FILE
        ./src/pipe/pipe.c)
set(WORKER_FILE
        ./src/pipe/worker.c)
add_executable(HelloUv ${HELLO_FILE})
add_executable(IdleHandle ${IDLE_FILE})
add_executable(FsHandle ${FS_FILE})
add_executable(TcpHandle ${TCP_FILE})
add_executable(UdpHandle ${UDP_FILE})
add_executable(ProcessHandle ${PROCESS_FILE})
add_executable(ThreadHandle ${THREAD_FILE})
add_executable(DNSHandle ${DNS_FILE})
add_executable(PipeHandle ${PIPE_FILE})
add_executable(WorkerHandle ${WORKER_FILE})