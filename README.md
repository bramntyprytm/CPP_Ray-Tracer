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

Run the executable without an argument for final-quality output or use `--fast` for faster rendering.

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

## Building interactive Renderer on macOS

From the project root:

```bash
rm -rf build
mkdir build
cd build
cmake ..
cmake --build .
```

Run the CPU renderer:

```bash
./raytracer
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

Run the CPU renderer:

```powershell
.\Release\raytracer.exe
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

## Model files

Place OBJ files in the `models/` directory.

The default GPU scene uses:

```text
models/cow.obj
```

CMake copies the `models/` directory beside the executable after building. Code should therefore load the model using:

```cpp
"models/cow.obj"
```

Do not use machine-specific absolute paths.

## Changing cow size and position

The cow scale and position are controlled by the arguments passed to `load_obj_as_triangles`.

```cpp
load_obj_as_triangles(
    "models/cow.obj",
    scene,
    cow_material,
    0.045,
    Vec3(0.0, -0.5, -2.3)
);
```

The fourth argument is the scale:

```cpp
0.045
```

Use a smaller value to make the cow smaller and a larger value to make it larger.

The final argument is the translation:

```cpp
Vec3(x, y, z)
```

- `x` moves the model left or right;
- `y` moves the model up or down;
- `z` moves the model forward or backward.

## Rendering three parallel cows

The cow model's long body axis is aligned with the X axis. To place cows parallel and side by side, keep the same X position and vary Z:

```cpp
const double cow_scale = 0.045;
const double cow_x = 0.0;
const double cow_y = -0.5;

load_obj_as_triangles(
    "models/cow.obj",
    scene,
    cow_material,
    cow_scale,
    Vec3(cow_x, cow_y, -1.6)
);

load_obj_as_triangles(
    "models/cow.obj",
    scene,
    cow_material,
    cow_scale,
    Vec3(cow_x, cow_y, -2.3)
);

load_obj_as_triangles(
    "models/cow.obj",
    scene,
    cow_material,
    cow_scale,
    Vec3(cow_x, cow_y, -3.0)
);
```

## CPU renderer settings

Common render settings are defined in `src/main.cpp`:

```cpp
const int width = 800;
const int height = 450;
const int samples_per_pixel = 16;
const int max_depth = 5;
```

### Faster test renders

```cpp
const int width = 400;
const int height = 225;
const int samples_per_pixel = 1;
const int max_depth = 2;
```

### Higher-quality final renders

```cpp
const int width = 1280;
const int height = 720;
const int samples_per_pixel = 32;
const int max_depth = 5;
```

Higher sample counts and reflection depths increase render time.

## GPU performance settings

The current GPU preview performs a naive loop over every triangle for every primary, shadow, and reflected ray. Performance therefore falls quickly as more meshes are added.

Useful performance changes in the fragment shader include:

### Reduce reflection bounces

```glsl
const int max_bounces = 2;
```

### Keep the floor reflective but disable cow reflection rays

```glsl
float material_reflectivity(Hit hit) {
    if (hit.ground) {
        return 0.45;
    }

    return 0.0;
}
```

The cow can remain visually shiny through its specular term without spawning additional reflection rays.

### Only calculate shadows for the primary bounce

Use shadows on `bounce == 0` and skip them for reflected rays.

The next major optimisation would be a flattened GPU BVH or a compute-shader renderer.

## Output images

The CPU renderer writes PPM images. Depending on the current scene configuration, outputs may include files such as:

```text
renders/cow_naive.ppm
renders/cow_bvh_timed.ppm
renders/reflection_spheres.ppm
renders/comparison_1spp.ppm
renders/comparison_16spp.ppm
```

PPM files can be opened with image applications such as GIMP or converted to PNG for documentation or submission.

Example conversion with ImageMagick:

```bash
magick input.ppm output.png
```

## Implemented techniques

- parametric ray generation;
- ray-sphere intersection;
- Möller--Trumbore ray-triangle intersection;
- OBJ vertex and face parsing;
- polygon fan triangulation;
- closest-hit testing;
- ambient and diffuse lighting;
- Blinn--Phong specular highlights;
- hard shadow rays;
- reflected rays;
- random subpixel anti-aliasing;
- AABB intersection;
- CPU BVH construction and traversal;
- OpenGL fragment-shader ray tracing;
- interactive camera movement and mouse look.

## Troubleshooting

### CMake cannot find SDL2

SDL2 is optional. Configure CMake so that it uses:

```cmake
find_package(SDL2 QUIET)
```

and only creates the `interactive` target when `SDL2_FOUND` is true.

### `OpenGL/gl3.h` cannot be found on Windows

`OpenGL/gl3.h` is macOS-specific. Use conditional includes:

```cpp
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>
#else
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif
```

### `M_PI` is undefined with MSVC

Define a portable constant in `camera.h`:

```cpp
constexpr double PI = 3.14159265358979323846;
```

Replace all uses of `M_PI` with `PI`.

### GLAD fails to initialise

GLAD must be initialised after the GLFW OpenGL context is made current:

```cpp
glfwMakeContextCurrent(window);

#ifndef __APPLE__
if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialise GLAD\n";
    return 1;
}
#endif
```

### OBJ file cannot be opened

Confirm that:

```text
models/cow.obj
```

exists beside the executable. The CMake build should copy the `models/` directory using a post-build command.

### The cow is too large

Reduce the scale value in `load_obj_as_triangles`, for example:

```cpp
0.045
```

to:

```cpp
0.03
```

### Multiple cows appear nose-to-tail

The cow's body runs along the X axis. Keep X constant and vary Z to place the cows side by side.

## Suggested render evidence

Useful screenshots and tables include:

- gradient image;
- normal-coloured sphere;
- diffuse and specular sphere scene;
- hard shadow scene;
- reflective floor scene;
- 1 sample versus 16 samples per pixel;
- CPU cow render;
- naive versus BVH timing table;
- GPU preview with three reflective cows;
- MacBook versus RTX PC performance comparison.

## Known limitations

- GPU triangle traversal is currently naive;
- GPU mesh instances duplicate triangle data;
- the GPU preview has no anti-aliasing;
- cow triangles use flat face normals;
- the light is a point light and therefore produces hard shadows;
- texture mapping and refraction are not implemented;
- the CPU and GPU renderers use separate scene representations.

## Future work

- flattened GPU BVH traversal;
- compute-shader ray tracing;
- GPU mesh instancing;
- interpolated OBJ vertex normals;
- texture mapping;
- soft shadows;
- refraction;
- depth of field;
- path tracing and denoising.

## Licence and model attribution

Add the licence for your source code here.

Also add the source and licence information for each third-party OBJ model before submission.
