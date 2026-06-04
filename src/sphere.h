#ifndef SPHERE_H
#define SPHERE_H

#include <cmath>

#include "hittable.h"

struct Sphere {
    Vec3 center;
    double radius;
    Material material;

    Sphere() {}

    Sphere(const Vec3& center, double radius, const Material& material)
        : center(center), radius(radius), material(material) {}

    bool hit(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
        Vec3 oc = ray.origin - center;

        double a = dot(ray.direction, ray.direction);
        double b = 2.0 * dot(oc, ray.direction);
        double c = dot(oc, oc) - radius * radius;

        double discriminant = b * b - 4 * a * c;

        if (discriminant < 0) {
            return false;
        }

        double sqrt_d = std::sqrt(discriminant);

        double root = (-b - sqrt_d) / (2.0 * a);
        if (root < t_min || root > t_max) {
            root = (-b + sqrt_d) / (2.0 * a);

            if (root < t_min || root > t_max) {
                return false;
            }
        }

        rec.t = root;
        rec.point = ray.at(rec.t);
        rec.normal = normalize(rec.point - center);
        rec.material = material;

        return true;
    }
};

#endif