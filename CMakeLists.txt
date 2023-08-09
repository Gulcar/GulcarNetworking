cmake_minimum_required(VERSION 3.16)

project(GulcarNetworking)

add_library(${PROJECT_NAME}
    include/GulcarNet/Client.h
    include/GulcarNet/IPAddr.h
    include/GulcarNet/Other.h
    include/GulcarNet/Server.h
    src/Client.cpp
    src/IPAddr.cpp
    src/Server.cpp
    src/Socket.cpp
    src/Socket.h
    src/Transport.cpp
    src/Transport.h
)

target_include_directories(${PROJECT_NAME} PUBLIC
    include/
)

#add_compile_definitions(GULCAR_NET_SIM_PACKET_LOSS)
#add_compile_definitions(GULCAR_NET_DEBUG_OUT)

option(GULCAR_NET_COMPILE_EXAMPLE "Should the example for GulcarNetworking be compiled" OFF)

if (GULCAR_NET_COMPILE_EXAMPLE)
    add_subdirectory(example)
endif()
