cmake_minimum_required(VERSION 3.15)
project(crosshair VERSION 0.2.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# WIN32 selects the Windows subsystem: no console window.
add_executable(crosshair WIN32
    src/main.c
    src/overlay.c
)

target_link_libraries(crosshair PRIVATE user32 gdi32)
