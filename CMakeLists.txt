cmake_minimum_required(VERSION 3.5)

project(DRMUDEV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(DRMUDEV main.cpp)
target_include_directories(DRMUDEV PUBLIC /usr/include/libdrm)
target_link_libraries(DRMUDEV udev)
target_link_libraries(DRMUDEV drm)

