cmake_minimum_required(VERSION 3.20)

project(COSC3000RayTracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------- Offline CPU renderer ----------------

add_executable(raytracer
    src/main.cpp
)

target_include_directories(raytracer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---------------- Interactive SDL preview ----------------

find_package(SDL2 REQUIRED)

add_executable(interactive
    src/interactive.cpp
)

target_include_directories(interactive PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(interactive PRIVATE
    ${SDL2_LIBRARIES}
)

# ---------------- GPU / OpenGL preview ----------------

include(FetchContent)

FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/archive/refs/tags/3.4.zip
)

FetchContent_MakeAvailable(glfw)

add_executable(gpu_preview
    src/gpu/gpu_preview.cpp
)

target_include_directories(gpu_preview PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(gpu_preview PRIVATE
    glfw
)

if (APPLE)
    target_link_libraries(gpu_preview PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif (WIN32)
    target_link_libraries(gpu_preview PRIVATE opengl32)
else()
    target_link_libraries(gpu_preview PRIVATE GL)
endif()

# ---------------- Compiler warnings ----------------

if (MSVC)
    target_compile_options(raytracer PRIVATE /W4)
    target_compile_options(interactive PRIVATE /W4)
    target_compile_options(gpu_preview PRIVATE /W4)
else()
    target_compile_options(raytracer PRIVATE -Wall -Wextra)
    target_compile_options(interactive PRIVATE -Wall -Wextra)
    target_compile_options(gpu_preview PRIVATE -Wall -Wextra)
endif()