cmake_minimum_required(VERSION 3.12)
project(use_cppcoro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
find_package(cppcoro REQUIRED)

add_executable(use_cppcoro use_cppcoro.cpp)
target_link_libraries(use_cppcoro PRIVATE cppcoro::cppcoro)
