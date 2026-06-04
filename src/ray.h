#ifndef RAY_H
#define RAY_H

#include "vec3.h"

struct Ray {
    Vec3 origin;
    Vec3 direction;

    Ray() {}

    Ray(const Vec3& origin, const Vec3& direction)
        : origin(origin), direction(direction) {}

    Vec3 at(double t) const {
        return origin + t * direction;
    }
};

#endif