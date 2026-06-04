#ifndef BVH_H
#define BVH_H

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "aabb.h"
#include "hittable.h"
#include "triangle.h"

struct BVHNode {
    AABB box;
    std::unique_ptr<BVHNode> left;
    std::unique_ptr<BVHNode> right;

    int start;
    int end;
    bool leaf;

    BVHNode()
        : start(0), end(0), leaf(false) {}

    bool hit(
        const Ray& ray,
        const std::vector<Triangle>& triangles,
        double t_min,
        double t_max,
        HitRecord& rec
    ) const {
        if (!box.hit(ray, t_min, t_max)) {
            return false;
        }

        if (leaf) {
            bool hit_anything = false;
            HitRecord temp_rec;
            double closest_so_far = t_max;

            for (int i = start; i < end; i++) {
                if (triangles[i].hit(ray, t_min, closest_so_far, temp_rec)) {
                    hit_anything = true;
                    closest_so_far = temp_rec.t;
                    rec = temp_rec;
                }
            }

            return hit_anything;
        }

        HitRecord left_rec;
        HitRecord right_rec;

        bool hit_left = false;
        bool hit_right = false;

        if (left) {
            hit_left = left->hit(ray, triangles, t_min, t_max, left_rec);
        }

        if (right) {
            double right_t_max = hit_left ? left_rec.t : t_max;
            hit_right = right->hit(ray, triangles, t_min, right_t_max, right_rec);
        }

        if (hit_left && hit_right) {
            rec = right_rec.t < left_rec.t ? right_rec : left_rec;
            return true;
        }

        if (hit_left) {
            rec = left_rec;
            return true;
        }

        if (hit_right) {
            rec = right_rec;
            return true;
        }

        return false;
    }
};

inline bool compare_triangle_axis(const Triangle& a, const Triangle& b, int axis) {
    Vec3 ca = a.centroid();
    Vec3 cb = b.centroid();

    if (axis == 0) {
        return ca.x < cb.x;
    }

    if (axis == 1) {
        return ca.y < cb.y;
    }

    return ca.z < cb.z;
}

inline AABB compute_triangle_range_box(
    const std::vector<Triangle>& triangles,
    int start,
    int end
) {
    AABB output_box = triangles[start].bounding_box();

    for (int i = start + 1; i < end; i++) {
        output_box = surrounding_box(output_box, triangles[i].bounding_box());
    }

    return output_box;
}

inline std::unique_ptr<BVHNode> build_bvh(
    std::vector<Triangle>& triangles,
    int start,
    int end
) {
    auto node = std::make_unique<BVHNode>();

    int object_count = end - start;

    node->box = compute_triangle_range_box(triangles, start, end);

    const int leaf_size = 4;

    if (object_count <= leaf_size) {
        node->start = start;
        node->end = end;
        node->leaf = true;
        return node;
    }

    AABB centroid_box(
        triangles[start].centroid(),
        triangles[start].centroid()
    );

    for (int i = start + 1; i < end; i++) {
        AABB point_box(triangles[i].centroid(), triangles[i].centroid());
        centroid_box = surrounding_box(centroid_box, point_box);
    }

    Vec3 extent = centroid_box.maximum - centroid_box.minimum;

    int axis = 0;
    if (extent.y > extent.x && extent.y > extent.z) {
        axis = 1;
    } else if (extent.z > extent.x && extent.z > extent.y) {
        axis = 2;
    }

    std::sort(
        triangles.begin() + start,
        triangles.begin() + end,
        [axis](const Triangle& a, const Triangle& b) {
            return compare_triangle_axis(a, b, axis);
        }
    );

    int mid = start + object_count / 2;

    node->left = build_bvh(triangles, start, mid);
    node->right = build_bvh(triangles, mid, end);
    node->leaf = false;

    return node;
}

#endif