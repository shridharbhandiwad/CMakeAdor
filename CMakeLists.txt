cmake_minimum_required(VERSION 3.10)

project(ComLib_C_Interface VERSION 1.0)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Define base directories
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

# Include header directory
include_directories(${INCLUDE_DIR})

# Gather all .c source files in src/
file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/*.c")



# Create the executable
add_executable(${PROJECT_NAME} extract_target_data.c ${SRC_FILES})

# Link Winsock library for UDP sockets
target_link_libraries(ComLib_C_Interface ws2_32)