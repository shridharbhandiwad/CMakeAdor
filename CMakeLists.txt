cmake_minimum_required(VERSION 3.10)

project(ComLib_C_Interface_Combined VERSION 1.0)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Define base directories
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

# Include header directory
include_directories(${INCLUDE_DIR})

# Gather all .c source files in src/ (if any exist)
file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/*.c")

# Create the combined executable
add_executable(${PROJECT_NAME} extract_combined_data.c ${SRC_FILES})

# Platform-specific libraries
if(WIN32)
    # Link Winsock library for UDP sockets on Windows
    target_link_libraries(${PROJECT_NAME} ws2_32)
endif()

# Set output name for convenience
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "extract_data")

# Print build information
message(STATUS "Building combined radar data extraction executable")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Output executable: extract_data")
if(WIN32)
    message(STATUS "Platform: Windows (linking ws2_32)")
else()
    message(STATUS "Platform: Unix/Linux")
endif()