cmake_minimum_required(VERSION 3.20)

project(COSC3000RayTracer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(raytracer
    src/main.cpp
)

target_include_directories(raytracer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(raytracer PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

find_package(SDL2 QUIET)

if (SDL2_FOUND)
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

    target_compile_definitions(interactive PRIVATE
        PROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )
else()
    message(STATUS "SDL2 not found; skipping interactive SDL preview.")
endif()

include(FetchContent)

FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/archive/refs/tags/3.4.zip
)

FetchContent_MakeAvailable(glfw)

set(GPU_PREVIEW_SOURCES
    src/gpu/gpu_preview.cpp
)

if (NOT APPLE)
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/glad/src/glad.c")
        list(APPEND GPU_PREVIEW_SOURCES external/glad/src/glad.c)
    else()
        message(FATAL_ERROR
            "GLAD was not found. Expected external/glad/src/glad.c and "
            "external/glad/include/glad/glad.h on Windows/Linux."
        )
    endif()
endif()

add_executable(gpu_preview
    ${GPU_PREVIEW_SOURCES}
)

target_include_directories(gpu_preview PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (NOT APPLE)
    target_include_directories(gpu_preview PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
    )
endif()

target_compile_definitions(gpu_preview PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(gpu_preview PRIVATE glfw)

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

if (MSVC)
    target_compile_options(raytracer PRIVATE /W4)
    target_compile_options(gpu_preview PRIVATE /W4)
    if (SDL2_FOUND)
        target_compile_options(interactive PRIVATE /W4)
    endif()
else()
    target_compile_options(raytracer PRIVATE -Wall -Wextra)
    target_compile_options(gpu_preview PRIVATE -Wall -Wextra)
    if (SDL2_FOUND)
        target_compile_options(interactive PRIVATE -Wall -Wextra)
    endif()
endif()
