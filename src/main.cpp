#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "camera.h"
#include "obj_loader.h"
#include "scene.h"
#include "vec3.h"

namespace fs = std::filesystem;

#ifndef PROJECT_ROOT_DIR
#define PROJECT_ROOT_DIR "."
#endif

struct RenderSettings {
    int width;
    int height;
    int samples_per_pixel;
    int max_depth;
};

struct BenchmarkResult {
    std::string model;
    std::size_t triangles;
    int width;
    int height;
    int samples;
    int max_depth;
    double naive_seconds;
    double bvh_seconds;
};

static fs::path project_root() {
    return fs::path(PROJECT_ROOT_DIR);
}

static fs::path render_directory() {
    fs::path output = project_root() / "renders";
    fs::create_directories(output);
    return output;
}

static fs::path model_directory() {
    return project_root() / "models";
}

static std::ofstream open_output_file(const fs::path& path, std::ios::openmode mode = std::ios::out) {
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path());
    }

    std::ofstream out(path, mode);
    if (!out.is_open()) {
        throw std::runtime_error("Could not create output file: " + path.string());
    }

    std::cout << "Saving: " << fs::absolute(path) << "\n";
    return out;
}

static Vec3 background_colour(const Ray& ray) {
    Vec3 unit_direction = normalize(ray.direction);
    double a = 0.5 * (unit_direction.y + 1.0);
    return (1.0 - a) * Vec3(1.0, 1.0, 1.0) + a * Vec3(0.5, 0.7, 1.0);
}

static Vec3 ray_colour(const Ray& ray, const Scene& scene, int depth) {
    if (depth <= 0) {
        return Vec3(0, 0, 0);
    }

    HitRecord rec;
    if (!scene.hit(ray, 0.001, 1e30, rec)) {
        return background_colour(ray);
    }

    const Vec3 light_position(2, 2, 0);
    const Vec3 light_direction = normalize(light_position - rec.point);
    const Vec3 view_direction = normalize(-ray.direction);

    double diffuse = std::max(dot(rec.normal, light_direction), 0.0);

    Ray shadow_ray(rec.point + rec.normal * 0.001, light_direction);
    const double light_distance = (light_position - rec.point).length();

    HitRecord shadow_rec;
    const bool in_shadow = scene.hit(shadow_ray, 0.001, light_distance, shadow_rec);

    double specular = 0.0;
    if (!in_shadow) {
        Vec3 halfway_direction = normalize(light_direction + view_direction);
        specular = std::pow(
            std::max(dot(rec.normal, halfway_direction), 0.0),
            rec.material.shininess
        );
    } else {
        diffuse = 0.0;
    }

    Vec3 diffuse_colour = rec.material.colour *
        (rec.material.ambient + rec.material.diffuse_strength * diffuse);

    Vec3 specular_colour = Vec3(1.0, 1.0, 1.0) *
        (rec.material.specular_strength * specular);

    Vec3 local_colour = diffuse_colour + specular_colour;

    if (rec.material.reflectivity <= 0.0) {
        return local_colour;
    }

    Vec3 reflected_direction = reflect(normalize(ray.direction), rec.normal);
    Ray reflected_ray(rec.point + rec.normal * 0.001, reflected_direction);
    Vec3 reflected_colour = ray_colour(reflected_ray, scene, depth - 1);

    return local_colour * (1.0 - rec.material.reflectivity)
         + reflected_colour * rec.material.reflectivity;
}

static double render_image(
    const fs::path& output_path,
    const Camera& camera,
    const RenderSettings& settings,
    const std::function<Vec3(const Ray&)>& shader
) {
    auto start_time = std::chrono::high_resolution_clock::now();
    std::ofstream out = open_output_file(output_path);
    out << "P3\n" << settings.width << " " << settings.height << "\n255\n";

    for (int y = settings.height - 1; y >= 0; --y) {
        std::cerr << "\rRendering " << output_path.filename().string()
                  << " | scanlines remaining: " << y << " " << std::flush;

        for (int x = 0; x < settings.width; ++x) {
            Vec3 colour(0, 0, 0);

            for (int sample = 0; sample < settings.samples_per_pixel; ++sample) {
                double u = double(x + random_double()) / double(settings.width - 1);
                double v = double(y + random_double()) / double(settings.height - 1);
                colour += shader(camera.get_ray(u, v));
            }

            colour = colour / double(settings.samples_per_pixel);

            int r = int(255.999 * clamp(colour.x, 0.0, 1.0));
            int g = int(255.999 * clamp(colour.y, 0.0, 1.0));
            int b = int(255.999 * clamp(colour.z, 0.0, 1.0));
            out << r << " " << g << " " << b << "\n";
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    std::cerr << "\nCompleted " << output_path.filename().string()
              << " in " << elapsed.count() << " seconds.\n";
    return elapsed.count();
}

static double render_scene(
    const fs::path& output_path,
    const Scene& scene,
    const Camera& camera,
    const RenderSettings& settings
) {
    return render_image(
        output_path,
        camera,
        settings,
        [&scene, &settings](const Ray& ray) {
            return ray_colour(ray, scene, settings.max_depth);
        }
    );
}

static Camera make_camera(int width, int height) {
    Camera camera;
    camera.position = Vec3(0.0, 0.1, 0.6);
    camera.yaw = -90.0;
    camera.pitch = -3.0;
    camera.fov_degrees = 60.0;
    camera.aspect_ratio = double(width) / double(height);
    return camera;
}

static Material make_red(double reflectivity = 0.0) {
    return Material(Vec3(1.0, 0.2, 0.2), 0.1, 1.0, 0.7, 64.0, reflectivity);
}

static Material make_blue(double reflectivity = 0.0) {
    return Material(Vec3(0.2, 0.3, 1.0), 0.1, 1.0, 0.8, 96.0, reflectivity);
}

static Material make_green(double reflectivity = 0.0) {
    return Material(Vec3(0.2, 0.8, 0.3), 0.1, 1.0, 0.5, 32.0, reflectivity);
}

static Material make_ground(double reflectivity) {
    return Material(Vec3(0.5, 0.5, 0.5), 0.1, 1.0, 0.2, 24.0, reflectivity);
}

static Material make_cow(double reflectivity = 0.05) {
    return Material(Vec3(0.75, 0.75, 0.75), 0.15, 1.0, 0.55, 64.0, reflectivity);
}

static void add_sphere_scene(Scene& scene, bool reflective) {
    scene.add_sphere(Sphere(Vec3(0, 0, -1), 0.5, make_red(reflective ? 0.15 : 0.0)));
    scene.add_sphere(Sphere(Vec3(-1.0, 0, -1.5), 0.5, make_blue(reflective ? 0.25 : 0.0)));
    scene.add_sphere(Sphere(Vec3(1.0, 0, -1.5), 0.5, make_green(reflective ? 0.10 : 0.0)));
    scene.add_sphere(Sphere(Vec3(0, -100.5, -1), 100.0, make_ground(reflective ? 0.50 : 0.0)));
}

static bool add_cow(
    Scene& scene,
    const Vec3& offset,
    double scale,
    double reflectivity = 0.05
) {
    const fs::path cow_path = model_directory() / "cow.obj";
    return load_obj_as_triangles(
        cow_path.string(),
        scene,
        make_cow(reflectivity),
        scale,
        offset
    );
}

static Scene make_cow_scene(bool reflective_ground, bool reflective_cow) {
    Scene scene;

    scene.add_sphere(Sphere(
        Vec3(0, -100.5, -1),
        100.0,
        make_ground(reflective_ground ? 0.70 : 0.0)
    ));

    if (!add_cow(
        scene,
        Vec3(0.0, -0.5, -2.5),
        0.001,
        reflective_cow ? 0.12 : 0.0
    )) {
        throw std::runtime_error("Could not load models/cow.obj");
    }

    scene.build_triangle_bvh();
    scene.set_use_bvh(true);

    return scene;
}

static void render_gradient(const fs::path& output_path, const RenderSettings& settings) {
    Camera camera = make_camera(settings.width, settings.height);
    render_image(
        output_path,
        camera,
        settings,
        [](const Ray& ray) { return background_colour(ray); }
    );
}

static void render_normal_sphere(const fs::path& output_path, const RenderSettings& settings) {
    Camera camera;
    camera.position = Vec3(0, 0, 0);
    camera.yaw = -90.0;
    camera.pitch = 0.0;
    camera.fov_degrees = 90.0;
    camera.aspect_ratio = double(settings.width) / double(settings.height);

    render_image(
        output_path,
        camera,
        settings,
        [](const Ray& ray) {
            const Vec3 centre(0, 0, -1);
            const double radius = 0.5;
            Vec3 oc = ray.origin - centre;
            double a = dot(ray.direction, ray.direction);
            double b = 2.0 * dot(oc, ray.direction);
            double c = dot(oc, oc) - radius * radius;
            double discriminant = b * b - 4.0 * a * c;

            if (discriminant >= 0.0) {
                double t = (-b - std::sqrt(discriminant)) / (2.0 * a);
                if (t > 0.001) {
                    Vec3 normal = normalize(ray.at(t) - centre);
                    return 0.5 * Vec3(normal.x + 1.0, normal.y + 1.0, normal.z + 1.0);
                }
            }
            return background_colour(ray);
        }
    );
}

static BenchmarkResult benchmark_cow(const RenderSettings& settings) {
    Scene scene;
    scene.add_sphere(Sphere(Vec3(0, -100.5, -1), 100.0, make_ground(0.25)));

    if (!add_cow(scene, Vec3(0.0, -0.5, -2.5), 0.001, 0.05)) {
        throw std::runtime_error("Could not load models/cow.obj");
    }

    const std::size_t triangle_count = scene.triangles.size();
    scene.build_triangle_bvh();
    Camera camera = make_camera(settings.width, settings.height);

    scene.set_use_bvh(false);
    double naive = render_scene(render_directory() / "cow_naive.ppm", scene, camera, settings);

    scene.set_use_bvh(true);
    double bvh = render_scene(render_directory() / "cow_bvh.ppm", scene, camera, settings);

    return BenchmarkResult{
        "cow.obj",
        triangle_count,
        settings.width,
        settings.height,
        settings.samples_per_pixel,
        settings.max_depth,
        naive,
        bvh
    };
}

static void write_benchmark_csv(const std::vector<BenchmarkResult>& results) {
    fs::path csv_path = render_directory() / "performance_results.csv";
    std::ofstream out = open_output_file(csv_path);
    out << "Model,Triangles,Width,Height,Samples,MaxDepth,NaiveSeconds,BVHSeconds,Speedup\n";

    out << std::fixed << std::setprecision(6);
    for (const BenchmarkResult& result : results) {
        double speedup = result.bvh_seconds > 0.0
            ? result.naive_seconds / result.bvh_seconds
            : 0.0;

        out << result.model << ','
            << result.triangles << ','
            << result.width << ','
            << result.height << ','
            << result.samples << ','
            << result.max_depth << ','
            << result.naive_seconds << ','
            << result.bvh_seconds << ','
            << speedup << '\n';
    }
}

static void write_render_manifest() {
    fs::path manifest_path = render_directory() / "RENDER_ASSETS.txt";
    std::ofstream out = open_output_file(manifest_path);
    out << "Generated CPU render assets\n"
        << "===========================\n"
        << "gradient.ppm\n"
        << "sphere_normals.ppm\n"
        << "specular_spheres.ppm\n"
        << "reflection_spheres.ppm\n"
        << "comparison_1spp.ppm\n"
        << "comparison_16spp.ppm\n"
        << "cow_cpu_render.ppm\n"
        << "cow_mesh.ppm\n"
        << "reflection_scene.ppm\n"
        << "complex_model.ppm (three-cow stress scene)\n"
        << "cow_naive.ppm\n"
        << "cow_bvh.ppm\n"
        << "performance_results.csv\n\n"
        << "Run tools/convert_ppm_to_png.py to create the PNG files used by Overleaf.\n"
        << "The GPU screenshot gpu_three_cows.ppm is created by pressing P in gpu_preview.\n";
}

static void run_render_suite(bool final_quality) {
    const RenderSettings quick = final_quality
        ? RenderSettings{800, 450, 4, 4}
        : RenderSettings{400, 225, 1, 2};

    const RenderSettings antialias_high = final_quality
        ? RenderSettings{800, 450, 16, 5}
        : RenderSettings{400, 225, 8, 3};

    const RenderSettings benchmark_settings{400, 225, 1, 2};
    Camera camera = make_camera(quick.width, quick.height);

    render_gradient(render_directory() / "gradient.ppm", quick);
    render_normal_sphere(render_directory() / "sphere_normals.ppm", quick);

    {
        Scene scene;
        add_sphere_scene(scene, false);
        render_scene(render_directory() / "specular_spheres.ppm", scene, camera, quick);
    }

    {
        Scene scene;
        add_sphere_scene(scene, true);
        render_scene(render_directory() / "reflection_spheres.ppm", scene, camera, quick);

        RenderSettings one_sample = quick;
        one_sample.samples_per_pixel = 1;
        render_scene(render_directory() / "comparison_1spp.ppm", scene, camera, one_sample);
        render_scene(render_directory() / "comparison_16spp.ppm", scene, camera, antialias_high);
    }

    {
        Scene scene = make_cow_scene(true, false);

        render_scene(
            render_directory() / "cow_cpu_render.ppm",
            scene,
            camera,
            quick
        );
        render_scene(render_directory() / "cow_mesh.ppm", scene, camera, quick);
    }

    {
        Scene scene = make_cow_scene(true, true);
        render_scene(render_directory() / "reflection_scene.ppm", scene, camera, quick);
    }

    {
        Scene scene;
        scene.add_sphere(Sphere(Vec3(0, -100.5, -1), 100.0, make_ground(0.35)));
        if (!add_cow(scene, Vec3(0.0, -0.5, -1.6), 0.001, 0.10) ||
            !add_cow(scene, Vec3(0.0, -0.5, -2.3), 0.001, 0.10) ||
            !add_cow(scene, Vec3(0.0, -0.5, -3.0), 0.001, 0.10)) {
            throw std::runtime_error("Could not create the three-cow render scene.");
        }
        scene.build_triangle_bvh();
        scene.set_use_bvh(true);
        render_scene(render_directory() / "complex_model.ppm", scene, camera, quick);
    }

    write_benchmark_csv({benchmark_cow(benchmark_settings)});
    write_render_manifest();

    std::cout << "\nRender suite completed.\n"
              << "PPM files: " << fs::absolute(render_directory()) << "\n"
              << "Next: run tools/convert_ppm_to_png.py\n";
}

static void print_usage(const char* executable) {
    std::cout
        << "Usage:\n"
        << "  " << executable << "              Generate higher-quality render assets\n"
        << "  " << executable << " --fast       Generate render assets quickly\n"
        << "  " << executable << " --benchmark  Generate cow naive/BVH timing files only\n";
}

int main(int argc, char** argv) {
    try {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));

        std::cout << "Project root: " << fs::absolute(project_root()) << "\n";
        std::cout << "Render folder: " << fs::absolute(render_directory()) << "\n";

        if (argc == 1) {
            run_render_suite(true);
        } else if (argc == 2 && std::string(argv[1]) == "--fast") {
            run_render_suite(false);
        } else if (argc == 2 && std::string(argv[1]) == "--benchmark") {
            write_benchmark_csv({benchmark_cow(RenderSettings{400, 225, 1, 2})});
        } else {
            print_usage(argv[0]);
            return 1;
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << "\n";
        return 1;
    }
}
