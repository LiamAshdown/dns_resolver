cmake_minimum_required(VERSION 4.1)
project(dns_resolver)

add_compile_definitions(WIN32_LEAN_AND_MEAN)

set(CMAKE_CXX_STANDARD 23)

add_executable(dns_resolver main.cpp)
