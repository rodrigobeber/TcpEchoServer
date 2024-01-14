cmake_minimum_required(VERSION 3.10)
project(EchoServer VERSION 1.0)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(include)

# Automatically add all source files from the src directory
file(GLOB SOURCES "src/*.cpp")

# Define the executable
add_executable(echo-server ${SOURCES})
