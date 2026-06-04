#ifndef MATERIAL_H
#define MATERIAL_H

#include "vec3.h"

struct Material {
    Vec3 colour;
    double ambient;
    double diffuse_strength;
    double specular_strength;
    double shininess;
    double reflectivity;

    Material()
        : colour(1, 1, 1),
          ambient(0.1),
          diffuse_strength(1.0),
          specular_strength(0.5),
          shininess(32.0),
          reflectivity(0.0) {}

    Material(
        const Vec3& colour,
        double ambient,
        double diffuse_strength,
        double specular_strength,
        double shininess,
        double reflectivity
    )
        : colour(colour),
          ambient(ambient),
          diffuse_strength(diffuse_strength),
          specular_strength(specular_strength),
          shininess(shininess),
          reflectivity(reflectivity) {}
};

#endif