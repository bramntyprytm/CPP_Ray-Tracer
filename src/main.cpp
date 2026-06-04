#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <chrono>

#include "obj_loader.h"
#include "scene.h"
#include "vec3.h"
#include "camera.h"

// ---------------- Shading ----------------

Vec3 ray_colour(const Ray& ray, const Scene& scene, int depth) {
    if (depth <= 0) {
        return Vec3(0, 0, 0);
    }

    HitRecord rec;

    if (scene.hit(ray, 0.001, 1e30, rec)) {
        Vec3 light_position(2, 2, 0);
        Vec3 light_direction = normalize(light_position - rec.point);

        Vec3 view_direction = normalize(-ray.direction);

        double ambient = rec.material.ambient;

        double diffuse = dot(rec.normal, light_direction);
        if (diffuse < 0) {
            diffuse = 0;
        }

        Ray shadow_ray(rec.point + rec.normal * 0.001, light_direction);
        double light_distance = (light_position - rec.point).length();

        HitRecord shadow_rec;
        bool in_shadow = scene.hit(shadow_ray, 0.001, light_distance, shadow_rec);

        double specular = 0.0;

        if (!in_shadow) {
            Vec3 halfway_direction = normalize(light_direction + view_direction);

            specular = dot(rec.normal, halfway_direction);
            if (specular < 0) {
                specular = 0;
            }

            specular = std::pow(specular, rec.material.shininess);
        }

        if (in_shadow) {
            diffuse = 0.0;
            specular = 0.0;
        }

        Vec3 diffuse_colour =
            rec.material.colour *
            (ambient + rec.material.diffuse_strength * diffuse);

        Vec3 specular_colour =
            Vec3(1.0, 1.0, 1.0) *
            (rec.material.specular_strength * specular);

        Vec3 local_colour = diffuse_colour + specular_colour;

        if (rec.material.reflectivity > 0.0) {
            Vec3 reflected_direction = reflect(normalize(ray.direction), rec.normal);
            Ray reflected_ray(rec.point + rec.normal * 0.001, reflected_direction);

            Vec3 reflected_colour = ray_colour(reflected_ray, scene, depth - 1);

            return local_colour * (1.0 - rec.material.reflectivity)
                 + reflected_colour * rec.material.reflectivity;
        }

        return local_colour;
    }

    Vec3 unit_direction = normalize(ray.direction);
    double a = 0.5 * (unit_direction.y + 1.0);

    Vec3 white(1.0, 1.0, 1.0);
    Vec3 blue(0.5, 0.7, 1.0);

    return (1.0 - a) * white + a * blue;
}

double render_scene(
    const std::string& output_filename,
    const Scene& scene,
    const Camera& camera,
    int width,
    int height,
    int samples_per_pixel,
    int max_depth
) {
    auto start_time = std::chrono::high_resolution_clock::now();

    std::ofstream out(output_filename);
    out << "P3\n" << width << " " << height << "\n255\n";

    for (int y = height - 1; y >= 0; y--) {
        std::cerr << "\rRendering " << output_filename
                  << " | scanlines remaining: " << y << " "
                  << std::flush;

        for (int x = 0; x < width; x++) {
            Vec3 colour(0, 0, 0);

            for (int s = 0; s < samples_per_pixel; s++) {
                double u = double(x + random_double()) / double(width - 1);
                double v = double(y + random_double()) / double(height - 1);

                Ray ray = camera.get_ray(u, v);

                colour += ray_colour(ray, scene, max_depth);
            }

            colour = colour / double(samples_per_pixel);

            int r = int(255.999 * clamp(colour.x, 0.0, 1.0));
            int g = int(255.999 * clamp(colour.y, 0.0, 1.0));
            int b = int(255.999 * clamp(colour.z, 0.0, 1.0));

            out << r << " " << g << " " << b << "\n";
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cerr << "\nDone. Rendered " << output_filename
              << " in " << elapsed.count() << " seconds.\n";

    return elapsed.count();
}

// ---------------- Main ----------------

int main() {
    std::srand(std::time(nullptr));

    const int width = 400;
    const int height = 225;
    const int samples_per_pixel = 1;
    const int max_depth = 2;

    Scene scene;

    Material red(
        Vec3(1.0, 0.2, 0.2),
        0.1,
        1.0,
        0.7,
        64.0,
        0.15
    );

    Material blue(
        Vec3(0.2, 0.3, 1.0),
        0.1,
        1.0,
        0.8,
        96.0,
        0.25
    );

    Material green(
        Vec3(0.2, 0.8, 0.3),
        0.1,
        1.0,
        0.5,
        32.0,
        0.10
    );

    Material grey(
        Vec3(0.5, 0.5, 0.5),
        0.1,
        1.0,
        0.2,
        24.0,
        0.45
    );

    Material cow_material(
        Vec3(0.75, 0.75, 0.75),
        0.15,
        1.0,
        0.25,
        32.0,
        0.05
    );

    scene.add_sphere(Sphere(Vec3(0, 0, -1), 0.5, red));
    scene.add_sphere(Sphere(Vec3(-1.0, 0, -1.5), 0.5, blue));
    scene.add_sphere(Sphere(Vec3(1.0, 0, -1.5), 0.5, green));
    scene.add_sphere(Sphere(Vec3(0, -100.5, -1), 100.0, grey));

    load_obj_as_triangles(
        "../models/cow.obj",
        scene,
        cow_material,
        0.4,
        Vec3(0.0, -0.5, -2.5)
    );

    scene.build_triangle_bvh();

    Camera camera_front;
    camera_front.position = Vec3(0, 0, 0);
    camera_front.yaw = -90.0;
    camera_front.pitch = 0.0;
    camera_front.fov_degrees = 60.0;
    camera_front.aspect_ratio = double(width) / double(height);

    double front_time = render_scene(
        "../renders/camera_front.ppm",
        scene,
        camera_front,
        width,
        height,
        samples_per_pixel,
        max_depth
    );

    Camera camera_left;
    camera_left.position = Vec3(-2.0, 0.2, 0.5);
    camera_left.yaw = -55.0;
    camera_left.pitch = -5.0;
    camera_left.fov_degrees = 60.0;
    camera_left.aspect_ratio = double(width) / double(height);

    double left_time = render_scene(
        "../renders/camera_left.ppm",
        scene,
        camera_left,
        width,
        height,
        samples_per_pixel,
        max_depth
    );

    Camera camera_high;
    camera_high.position = Vec3(0.0, 1.5, 1.0);
    camera_high.yaw = -90.0;
    camera_high.pitch = -25.0;
    camera_high.fov_degrees = 65.0;
    camera_high.aspect_ratio = double(width) / double(height);

    double high_time = render_scene(
        "../renders/camera_high.ppm",
        scene,
        camera_high,
        width,
        height,
        samples_per_pixel,
        max_depth
    );

    std::cout << "\nCamera test renders\n";
    std::cout << "-------------------\n";
    std::cout << "Front camera: " << front_time << " seconds\n";
    std::cout << "Left camera:  " << left_time << " seconds\n";
    std::cout << "High camera:  " << high_time << " seconds\n";

    return 0;
}