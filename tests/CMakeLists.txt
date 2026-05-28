add_executable(vfx_engine_tests "main.cpp")
target_link_libraries(vfx_engine_tests PRIVATE vfx_engine)
add_test(NAME ThreadingAndRHITest COMMAND vfx_engine_tests)
