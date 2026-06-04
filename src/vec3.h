#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <cstdlib>

struct Vec3 {
    double x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}

    Vec3 operator-() const {
        return Vec3(-x, -y, -z);
    }

    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    Vec3 operator*(double t) const {
        return Vec3(x * t, y * t, z * t);
    }

    Vec3 operator/(double t) const {
        return Vec3(x / t, y / t, z / t);
    }

    Vec3& operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    double length_squared() const {
        return x * x + y * y + z * z;
    }

    double length() const {
        return std::sqrt(length_squared());
    }
};

inline Vec3 operator*(double t, const Vec3& v) {
    return Vec3(t * v.x, t * v.y, t * v.z);
}

inline Vec3 operator*(const Vec3& a, const Vec3& b) {
    return Vec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

inline double dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

inline Vec3 normalize(const Vec3& v) {
    return v / v.length();
}

inline Vec3 reflect(const Vec3& v, const Vec3& normal) {
    return v - 2.0 * dot(v, normal) * normal;
}

inline double clamp(double value, double min_value, double max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

inline double random_double() {
    return double(std::rand()) / double(RAND_MAX);
}

#endif