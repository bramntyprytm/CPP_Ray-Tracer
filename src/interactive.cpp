#include <SDL.h>

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

#include "camera.h"
#include "obj_loader.h"
#include "scene.h"
#include "vec3.h"

Vec3 ray_colour_preview(const Ray& ray, const Scene& scene) {
    HitRecord rec;

    if (scene.hit(ray, 0.001, 1e30, rec)) {
        Vec3 light_position(2, 2, 0);
        Vec3 light_direction = normalize(light_position - rec.point);

        double diffuse = dot(rec.normal, light_direction);
        if (diffuse < 0) {
            diffuse = 0;
        }

        Ray shadow_ray(rec.point + rec.normal * 0.001, light_direction);
        double light_distance = (light_position - rec.point).length();

        HitRecord shadow_rec;
        bool in_shadow = scene.hit(shadow_ray, 0.001, light_distance, shadow_rec);

        if (in_shadow) {
            diffuse = 0.0;
        }

        double lighting = rec.material.ambient + rec.material.diffuse_strength * diffuse;

        return rec.material.colour * lighting;
    }

    Vec3 unit_direction = normalize(ray.direction);
    double a = 0.5 * (unit_direction.y + 1.0);

    Vec3 white(1.0, 1.0, 1.0);
    Vec3 blue(0.5, 0.7, 1.0);

    return (1.0 - a) * white + a * blue;
}

Uint32 colour_to_pixel(const Vec3& colour) {
    Uint8 r = Uint8(255.999 * clamp(colour.x, 0.0, 1.0));
    Uint8 g = Uint8(255.999 * clamp(colour.y, 0.0, 1.0));
    Uint8 b = Uint8(255.999 * clamp(colour.z, 0.0, 1.0));

    return (255 << 24) | (r << 16) | (g << 8) | b;
}

void render_preview(
    std::vector<Uint32>& pixels,
    const Scene& scene,
    const Camera& camera,
    int width,
    int height
) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double u = double(x) / double(width - 1);
            double v = double(height - 1 - y) / double(height - 1);

            Ray ray = camera.get_ray(u, v);
            Vec3 colour = ray_colour_preview(ray, scene);

            pixels[y * width + x] = colour_to_pixel(colour);
        }
    }
}

int main() {
    std::srand(std::time(nullptr));

    const int render_width = 320;
    const int render_height = 180;

    const int window_scale = 3;
    const int window_width = render_width * window_scale;
    const int window_height = render_height * window_scale;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Interactive Ray Tracer Preview",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width,
        window_height,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        render_width,
        render_height
    );

    if (!texture) {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::vector<Uint32> pixels(render_width * render_height);

    Scene scene;

    Material red(Vec3(1.0, 0.2, 0.2), 0.1, 1.0, 0.7, 64.0, 0.0);
    Material blue(Vec3(0.2, 0.3, 1.0), 0.1, 1.0, 0.8, 96.0, 0.0);
    Material green(Vec3(0.2, 0.8, 0.3), 0.1, 1.0, 0.5, 32.0, 0.0);
    Material grey(Vec3(0.5, 0.5, 0.5), 0.1, 1.0, 0.2, 24.0, 0.0);

    Material cow_material(Vec3(0.75, 0.75, 0.75), 0.15, 1.0, 0.25, 32.0, 0.0);

    scene.add_sphere(Sphere(Vec3(0, 0, -1), 0.5, red));
    scene.add_sphere(Sphere(Vec3(-1.0, 0, -1.5), 0.5, blue));
    scene.add_sphere(Sphere(Vec3(1.0, 0, -1.5), 0.5, green));
    scene.add_sphere(Sphere(Vec3(0, -100.5, -1), 100.0, grey));

    load_obj_as_triangles(
        "models/cow.obj",
        scene,
        cow_material,
        0.4,
        Vec3(0.0, -0.5, -2.5)
    );

    scene.build_triangle_bvh();
    scene.set_use_bvh(true);

    Camera camera;
    camera.position = Vec3(0, 0, 0);
    camera.yaw = -90.0;
    camera.pitch = 0.0;
    camera.fov_degrees = 60.0;
    camera.aspect_ratio = double(render_width) / double(render_height);

    bool running = true;

    const double move_speed = 0.08;
    const double look_speed = 2.0;

    std::cout << "Interactive preview started.\n";
    std::cout << "Controls: W/S/A/D move, Q/E down/up, arrow keys look, Esc quit.\n";

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        if (keys[SDL_SCANCODE_W]) {
            camera.position += camera.forward() * move_speed;
        }

        if (keys[SDL_SCANCODE_S]) {
            camera.position += camera.forward() * -move_speed;
        }

        if (keys[SDL_SCANCODE_A]) {
            camera.position += camera.right() * -move_speed;
        }

        if (keys[SDL_SCANCODE_D]) {
            camera.position += camera.right() * move_speed;
        }

        if (keys[SDL_SCANCODE_Q]) {
            camera.position.y -= move_speed;
        }

        if (keys[SDL_SCANCODE_E]) {
            camera.position.y += move_speed;
        }

        if (keys[SDL_SCANCODE_LEFT]) {
            camera.yaw -= look_speed;
        }

        if (keys[SDL_SCANCODE_RIGHT]) {
            camera.yaw += look_speed;
        }

        if (keys[SDL_SCANCODE_UP]) {
            camera.pitch += look_speed;
        }

        if (keys[SDL_SCANCODE_DOWN]) {
            camera.pitch -= look_speed;
        }

        if (camera.pitch > 89.0) {
            camera.pitch = 89.0;
        }

        if (camera.pitch < -89.0) {
            camera.pitch = -89.0;
        }

        render_preview(pixels, scene, camera, render_width, render_height);

        SDL_UpdateTexture(
            texture,
            nullptr,
            pixels.data(),
            render_width * sizeof(Uint32)
        );

        SDL_RenderClear(renderer);

        SDL_Rect destination;
        destination.x = 0;
        destination.y = 0;
        destination.w = window_width;
        destination.h = window_height;

        SDL_RenderCopy(renderer, texture, nullptr, &destination);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}