#ifndef SCENE_H
#define SCENE_H

#include <iostream>
#include <memory>
#include <vector>

#include "bvh.h"
#include "sphere.h"
#include "triangle.h"

struct Scene {
    std::vector<Sphere> spheres;
    std::vector<Triangle> triangles;

    std::unique_ptr<BVHNode> triangle_bvh;
    bool use_bvh = false;

    void set_use_bvh(bool enabled) {
        use_bvh = enabled && triangle_bvh != nullptr;
    }

    void add_sphere(const Sphere& sphere) {
        spheres.push_back(sphere);
    }

    void add_triangle(const Triangle& triangle) {
        triangles.push_back(triangle);
    }

    void build_triangle_bvh() {
        if (triangles.empty()) {
            use_bvh = false;
            return;
        }

        std::cerr << "Building triangle BVH for "
                  << triangles.size()
                  << " triangles...\n";

        triangle_bvh = build_bvh(triangles, 0, int(triangles.size()));
        use_bvh = true;

        std::cerr << "BVH build complete.\n";
    }

    bool hit_spheres(
        const Ray& ray,
        double t_min,
        double t_max,
        HitRecord& rec
    ) const {
        HitRecord temp_rec;
        bool hit_anything = false;
        double closest_so_far = t_max;

        for (const Sphere& sphere : spheres) {
            if (sphere.hit(ray, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }

    bool hit_triangles_naive(
        const Ray& ray,
        double t_min,
        double t_max,
        HitRecord& rec
    ) const {
        HitRecord temp_rec;
        bool hit_anything = false;
        double closest_so_far = t_max;

        for (const Triangle& triangle : triangles) {
            if (triangle.hit(ray, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }

    bool hit(
        const Ray& ray,
        double t_min,
        double t_max,
        HitRecord& rec
    ) const {
        HitRecord sphere_rec;
        HitRecord triangle_rec;

        bool hit_sphere = hit_spheres(ray, t_min, t_max, sphere_rec);

        double triangle_t_max = hit_sphere ? sphere_rec.t : t_max;

        bool hit_triangle = false;

        if (use_bvh && triangle_bvh) {
            hit_triangle = triangle_bvh->hit(
                ray,
                triangles,
                t_min,
                triangle_t_max,
                triangle_rec
            );
        } else {
            hit_triangle = hit_triangles_naive(
                ray,
                t_min,
                triangle_t_max,
                triangle_rec
            );
        }

        if (hit_sphere && hit_triangle) {
            rec = triangle_rec.t < sphere_rec.t ? triangle_rec : sphere_rec;
            return true;
        }

        if (hit_sphere) {
            rec = sphere_rec;
            return true;
        }

        if (hit_triangle) {
            rec = triangle_rec;
            return true;
        }

        return false;
    }
};

#endif