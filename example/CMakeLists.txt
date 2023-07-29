cmake_minimum_required(VERSION 3.16)

project(GulcarNetworkingExample)

add_executable(${PROJECT_NAME}
    example.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    GulcarNetworking
)

