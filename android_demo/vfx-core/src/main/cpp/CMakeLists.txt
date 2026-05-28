cmake_minimum_required(VERSION 3.22.1)
project("vfx-core")

set(CMAKE_CXX_STANDARD 20)

# Import the existing C++ VFX engine SDK
# Assuming it's compiled and installed or compiled from source.
# For this demo, we'll compile it directly from the parent repository sources.
set(VFX_ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../)

file(GLOB_RECURSE CORE_SOURCES "${VFX_ENGINE_DIR}/src/core/*.cpp")
file(GLOB_RECURSE RHI_SOURCES "${VFX_ENGINE_DIR}/src/rhi/*.cpp")
file(GLOB_RECURSE AUDIO_SOURCES "${VFX_ENGINE_DIR}/src/audio/*.cpp")
file(GLOB_RECURSE MEDIA_SOURCES "${VFX_ENGINE_DIR}/src/media/*.cpp")

add_library(vfx_engine SHARED
    ${CORE_SOURCES}
    ${RHI_SOURCES}
    ${AUDIO_SOURCES}
    ${MEDIA_SOURCES}
)
target_include_directories(vfx_engine PUBLIC ${VFX_ENGINE_DIR}/include)

add_library(vfx-jni SHARED vfx_jni.cpp)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(vfx-jni
    vfx_engine
    ${log-lib}
    ${android-lib}
)
