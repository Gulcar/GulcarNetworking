cmake_minimum_required(VERSION 3.16)

project(GulcarNetworking)

add_library(${PROJECT_NAME}
    
    include/GulcarNet/GulcarNet.h
    src/Socket.cpp
    include/GulcarNet/Socket.h
    src/IPAddr.cpp
    include/GulcarNet/IPAddr.h
)

target_include_directories(${PROJECT_NAME} PUBLIC
    include/
)

option(GULCAR_NET_COMPILE_EXAMPLE "Should the example for GulcarNetworking be compiled" OFF)

if (GULCAR_NET_COMPILE_EXAMPLE)
    add_subdirectory(example)
endif()
