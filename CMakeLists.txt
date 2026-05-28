cmake_minimum_required(VERSION 3.20)
project(VFXEngine VERSION 1.0.0 LANGUAGES CXX C)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable testing
enable_testing()

# Options
option(VFX_BUILD_TESTS "Build tests" ON)
option(VFX_BUILD_EXAMPLES "Build examples" ON)

# Include directories
set(VFX_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${VFX_INCLUDE_DIR})

# Add Subdirectories
add_subdirectory(src)

if(VFX_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(VFX_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
