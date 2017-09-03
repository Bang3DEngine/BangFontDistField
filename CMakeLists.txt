cmake_minimum_required(VERSION 2.8)

project(BangFontDistField C CXX)

set(CMAKE_BINARY_DIR bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wint-to-pointer-cast")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wswitch")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--export-dynamic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

set(BANG_ROOT /home/sephirot47/Bang)
include_directories(${BANG_ROOT}/include)
include_directories(${BANG_ROOT}/include/Bang)
include_directories(${BANG_ROOT}/include/Bang/BangCore)
include_directories(${BANG_ROOT}/include/Bang/BangEngine)
include_directories(${BANG_ROOT}/include/Bang/BangGraphics)
include_directories(${BANG_ROOT}/include/Bang/BangPreprocessor)

link_directories(${BANG_ROOT}/bin/Debug/lib)
link_libraries("Bang")

add_executable(BangFontDistField
Main.cpp
)

