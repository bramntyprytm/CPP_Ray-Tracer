# COSC3000 Ray Tracer

A ray tracer written from scratch in C++ for the COSC3000 major project.

The project contains:

- an offline CPU ray tracer that saves PPM images;
- OBJ triangle-mesh loading;
- Blinn--Phong lighting;
- hard shadows;
- reflections;
- anti-aliasing;
- CPU BVH acceleration;
- an OpenGL GPU preview with interactive camera movement;
- reflective OBJ cow rendering.

## Project structure

```text
COSC3000---Ray-Tracer/
├── CMakeLists.txt
├── external/
│   └── glad/
│       ├── include/
│       │   ├── glad/glad.h
│       │   └── KHR/khrplatform.h
│       └── src/glad.c
├── models/
│   └── cow.obj
├── renders/
├── src/
│   ├── main.cpp
│   ├── interactive.cpp
│   ├── camera.h
│   ├── vec3.h
│   ├── ray.h
│   ├── material.h
│   ├── hittable.h
│   ├── sphere.h
│   ├── triangle.h
│   ├── scene.h
│   ├── obj_loader.h
│   ├── aabb.h
│   ├── bvh.h
│   └── gpu/
│       └── gpu_preview.cpp
└── README.md
```

## Executables

The CMake project can build up to three executables.

### `raytracer`

Offline CPU renderer. It renders a scene to one or more `.ppm` files and can compare naive triangle traversal against BVH traversal.

The renderer supports the following command-line modes:

| Command | Description |
|---|---|
| `raytracer` | Runs the full final-quality render suite. |
| `raytracer --fast` | Runs a faster, reduced-quality render suite for development and testing. |
| `raytracer --benchmark` | Runs the rendering benchmark, including the naive traversal and BVH comparison. |

The previous `--report-final` and `--report-fast` arguments are no longer used. Run the executable without an argument for final-quality output or use `--fast` for faster rendering.

### `gpu_preview`

OpenGL GPU preview. It displays a ray-traced scene in a window and supports first-person camera movement.

### `interactive`

Optional SDL2 CPU preview. CMake skips this target when SDL2 is unavailable.

## Requirements

### Common requirements

- CMake 3.20 or newer
- A C++14 compiler
- Git, if cloning the repository

### macOS Interactive Build

- Xcode Command Line Tools
- Homebrew
- GLFW, downloaded automatically by CMake through `FetchContent`
- SDL2 only if the optional SDL preview is required

Install the basic tools:

```bash
xcode-select --install
brew install cmake
```

Optional SDL2 support:

```bash
brew install sdl2
```

macOS supports the OpenGL 4.1 fragment-shader preview. OpenGL compute shaders require OpenGL 4.3 and are therefore not used on macOS.

### Windows

- Visual Studio with **Desktop development with C++**
- Windows SDK
- CMake tools for Windows
- Current NVIDIA, AMD, or Intel graphics drivers

The Windows GPU build uses GLAD to load modern OpenGL functions. Confirm that these files exist:

```text
external/glad/include/glad/glad.h
external/glad/include/KHR/khrplatform.h
external/glad/src/glad.c
```

## Building the Interactive Renderer on macOS

From the project root:

```bash
rm -rf build
mkdir build
cd build
cmake ..
cmake --build .
```

Run the full final-quality CPU render suite:

```bash
./raytracer
```

Run the faster CPU render suite:

```bash
./raytracer --fast
```

Run the CPU benchmark:

```bash
./raytracer --benchmark
```

Run the GPU preview:

```bash
./gpu_preview
```

Run the optional SDL preview, if SDL2 was found:

```bash
./interactive
```

## Building on Windows

Open **Developer PowerShell for Visual Studio** and run the following from the project root:

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

Run the full final-quality CPU render suite:

```powershell
.\Release\raytracer.exe
```

Run the faster CPU render suite:

```powershell
.\Release\raytracer.exe --fast
```

Run the CPU benchmark:

```powershell
.\Release\raytracer.exe --benchmark
```

Run the GPU preview:

```powershell
.\Release\gpu_preview.exe
```

When the GPU preview starts, the terminal should print the OpenGL version and renderer. On an NVIDIA PC, the renderer line should identify the NVIDIA GPU.

## Controls for `gpu_preview`

| Input | Action |
|---|---|
| `W` | Move forward |
| `S` | Move backward |
| `A` | Move left |
| `D` | Move right |
| `Q` | Move down |
| `E` | Move up |
| Mouse | Look around |
| `Tab` | Release or capture the mouse, if enabled |
| `Esc` | Quit |