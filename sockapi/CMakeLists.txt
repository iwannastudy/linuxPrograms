cmake_minimum_required(VERSION 3.0.0)
project(test_cmake VERSION 0.1.0 LANGUAGES C)

include_directories(libsrc)
#add_subdirectory(test)

add_executable(tcp_server libsrc/etcp.c test/tcpserver.c)
add_executable(udp_server libsrc/etcp.c test/udpserver.c)
add_executable(udp_client libsrc/etcp.c test/udpclient.c)
add_executable(tcp_client libsrc/etcp.c test/tcpclient.c)