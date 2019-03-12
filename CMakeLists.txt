cmake_minimum_required(VERSION 3.12)
project(tp2 C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "-pthread -Wall")

include_directories(common)

file(GLOB CLIENT_SRC client/*.c client/*.h)
file(GLOB SERVER_SRC server/*.c server/*.h)
file(GLOB COMMON_SRC common/*.c common/*.h)

add_executable(tp2_server ${SERVER_SRC} ${COMMON_SRC})
add_executable(tp2_client ${CLIENT_SRC} ${COMMON_SRC})