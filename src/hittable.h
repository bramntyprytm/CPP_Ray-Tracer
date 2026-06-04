#ifndef HITTABLE_H
#define HITTABLE_H

#include "material.h"
#include "ray.h"
#include "vec3.h"

struct HitRecord {
    double t;
    Vec3 point;
    Vec3 normal;
    Material material;
};

#endif