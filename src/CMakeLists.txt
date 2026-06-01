file(GLOB_RECURSE CORE_SOURCES "core/*.cpp")
file(GLOB_RECURSE RHI_SOURCES "rhi/*.cpp")
file(GLOB_RECURSE AUDIO_SOURCES "audio/*.cpp")
file(GLOB_RECURSE MEDIA_SOURCES "media/*.cpp")

add_library(vfx_engine SHARED
    ${CORE_SOURCES}
    ${RHI_SOURCES}
    ${AUDIO_SOURCES}
    ${MEDIA_SOURCES}
)

target_include_directories(vfx_engine PUBLIC
    $<BUILD_INTERFACE:${VFX_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

find_package(Threads REQUIRED)
target_link_libraries(vfx_engine PRIVATE Threads::Threads)

if(ANDROID)
    target_link_libraries(vfx_engine PRIVATE
        android
        log
        EGL
        GLESv3
        mediandk
        oboe::oboe
    )
endif()

install(TARGETS vfx_engine
    EXPORT VFXEngineTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
