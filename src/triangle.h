#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <cmath>

#include "hittable.h"
#include "aabb.h"

struct Triangle {
    Vec3 v0, v1, v2;
    Material material;

    AABB bounding_box() const {
        const double epsilon = 0.0001;

        Vec3 minimum(
            std::min(v0.x, std::min(v1.x, v2.x)) - epsilon,
            std::min(v0.y, std::min(v1.y, v2.y)) - epsilon,
            std::min(v0.z, std::min(v1.z, v2.z)) - epsilon
        );

        Vec3 maximum(
            std::max(v0.x, std::max(v1.x, v2.x)) + epsilon,
            std::max(v0.y, std::max(v1.y, v2.y)) + epsilon,
            std::max(v0.z, std::max(v1.z, v2.z)) + epsilon
        );

        return AABB(minimum, maximum);
    }

    Vec3 centroid() const {
        return (v0 + v1 + v2) / 3.0;
    }

    Triangle() {}

    Triangle(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Material& material)
        : v0(v0), v1(v1), v2(v2), material(material) {}

    bool hit(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
        const double EPSILON = 1e-8;

        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;

        Vec3 h = cross(ray.direction, edge2);
        double a = dot(edge1, h);

        if (std::fabs(a) < EPSILON) {
            return false;
        }

        double f = 1.0 / a;
        Vec3 s = ray.origin - v0;
        double u = f * dot(s, h);

        if (u < 0.0 || u > 1.0) {
            return false;
        }

        Vec3 q = cross(s, edge1);
        double v = f * dot(ray.direction, q);

        if (v < 0.0 || u + v > 1.0) {
            return false;
        }

        double t = f * dot(edge2, q);

        if (t < t_min || t > t_max) {
            return false;
        }

        rec.t = t;
        rec.point = ray.at(t);

        Vec3 normal = normalize(cross(edge1, edge2));

        if (dot(normal, ray.direction) > 0) {
            normal = -normal;
        }

        rec.normal = normal;
        rec.material = material;

        return true;
    }
};

#endif