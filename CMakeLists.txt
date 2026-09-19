cmake_minimum_required(VERSION 3.15)
project(crosshair VERSION 0.4.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# WIN32 selects the Windows subsystem: no console window.
add_executable(crosshair WIN32
    src/main.c
    src/config.c
    src/overlay.c
)

target_compile_definitions(crosshair PRIVATE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(crosshair PRIVATE user32 gdi32)

# The parser is plain C, so it can be tested without any Win32 code.
option(CROSSHAIR_BUILD_TESTS "Build the config parser tests" OFF)
if(CROSSHAIR_BUILD_TESTS)
    enable_testing()
    add_executable(test_config tests/test_config.c src/config.c)
    target_include_directories(test_config PRIVATE src)
    target_compile_definitions(test_config PRIVATE _CRT_SECURE_NO_WARNINGS)
    add_test(NAME config COMMAND test_config)
endif()
