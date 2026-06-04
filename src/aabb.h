#ifndef AABB_H
#define AABB_H

#include <algorithm>

#include "ray.h"
#include "vec3.h"

struct AABB {
    Vec3 minimum;
    Vec3 maximum;

    AABB() {}

    AABB(const Vec3& minimum, const Vec3& maximum)
        : minimum(minimum), maximum(maximum) {}

    bool hit(const Ray& ray, double t_min, double t_max) const {
        for (int axis = 0; axis < 3; axis++) {
            double origin;
            double direction;
            double min_value;
            double max_value;

            if (axis == 0) {
                origin = ray.origin.x;
                direction = ray.direction.x;
                min_value = minimum.x;
                max_value = maximum.x;
            } else if (axis == 1) {
                origin = ray.origin.y;
                direction = ray.direction.y;
                min_value = minimum.y;
                max_value = maximum.y;
            } else {
                origin = ray.origin.z;
                direction = ray.direction.z;
                min_value = minimum.z;
                max_value = maximum.z;
            }

            double inv_d = 1.0 / direction;
            double t0 = (min_value - origin) * inv_d;
            double t1 = (max_value - origin) * inv_d;

            if (inv_d < 0.0) {
                std::swap(t0, t1);
            }

            if (t0 > t_min) {
                t_min = t0;
            }

            if (t1 < t_max) {
                t_max = t1;
            }

            if (t_max <= t_min) {
                return false;
            }
        }

        return true;
    }
};

inline AABB surrounding_box(const AABB& box0, const AABB& box1) {
    Vec3 small(
        std::min(box0.minimum.x, box1.minimum.x),
        std::min(box0.minimum.y, box1.minimum.y),
        std::min(box0.minimum.z, box1.minimum.z)
    );

    Vec3 big(
        std::max(box0.maximum.x, box1.maximum.x),
        std::max(box0.maximum.y, box1.maximum.y),
        std::max(box0.maximum.z, box1.maximum.z)
    );

    return AABB(small, big);
}

#endif