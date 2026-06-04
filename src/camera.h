#ifndef CAMERA_H
#define CAMERA_H

#include <cmath>

#include "ray.h"
#include "vec3.h"

constexpr double PI = 3.14159265358979323846;

struct Camera {
    Vec3 position;
    double yaw;
    double pitch;
    double fov_degrees;
    double aspect_ratio;

    Camera()
        : position(0, 0, 0),
          yaw(-90.0),
          pitch(0.0),
          fov_degrees(60.0),
          aspect_ratio(16.0 / 9.0) {}

    Vec3 forward() const {
        double yaw_rad = yaw * PI / 180.0;
        double pitch_rad = pitch * PI / 180.0;

        Vec3 direction;
        direction.x = std::cos(yaw_rad) * std::cos(pitch_rad);
        direction.y = std::sin(pitch_rad);
        direction.z = std::sin(yaw_rad) * std::cos(pitch_rad);

        return normalize(direction);
    }

    Vec3 right() const {
        Vec3 world_up(0, 1, 0);
        return normalize(cross(forward(), world_up));
    }

    Vec3 up() const {
        return normalize(cross(right(), forward()));
    }

    Ray get_ray(double u, double v) const {
        Vec3 f = forward();
        Vec3 r = right();
        Vec3 uvec = up();

        double theta = fov_degrees * PI / 180.0;
        double viewport_height = 2.0 * std::tan(theta / 2.0);
        double viewport_width = aspect_ratio * viewport_height;

        Vec3 horizontal = viewport_width * r;
        Vec3 vertical = viewport_height * uvec;

        Vec3 lower_left =
            position
            + f
            - horizontal / 2.0
            - vertical / 2.0;

        Vec3 direction =
            lower_left
            + u * horizontal
            + v * vertical
            - position;

        return Ray(position, direction);
    }
};

#endif