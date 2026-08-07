# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(OptionMarketMaker CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(option_mm
    src/main.cpp
    src/option_pricer.cpp
)

target_include_directories(option_mm PRIVATE include/)
