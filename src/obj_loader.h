#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "scene.h"

inline Vec3 transform_vertex(const Vec3& v, double scale, const Vec3& offset) {
    return v * scale + offset;
}

inline int parse_obj_vertex_index(const std::string& token) {
    std::stringstream ss(token);
    std::string index_string;

    std::getline(ss, index_string, '/');

    int index = std::stoi(index_string);

    return index - 1;
}

inline bool load_obj_as_triangles(
    const std::string& filename,
    Scene& scene,
    const Material& material,
    double scale,
    const Vec3& offset
) {
    std::ifstream file(filename);

    if (!file) {
        std::cerr << "Error: could not open OBJ file: " << filename << "\n";
        return false;
    }

    std::vector<Vec3> vertices;
    std::string line;

    int triangle_count = 0;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            double x, y, z;
            ss >> x >> y >> z;

            Vec3 vertex(x, y, z);
            vertices.push_back(transform_vertex(vertex, scale, offset));
        }
        else if (prefix == "f") {
            std::vector<int> indices;
            std::string token;

            while (ss >> token) {
                indices.push_back(parse_obj_vertex_index(token));
            }

            if (indices.size() < 3) {
                continue;
            }

            for (size_t i = 1; i + 1 < indices.size(); i++) {
                int i0 = indices[0];
                int i1 = indices[i];
                int i2 = indices[i + 1];

                if (
                    i0 < 0 || i0 >= int(vertices.size()) ||
                    i1 < 0 || i1 >= int(vertices.size()) ||
                    i2 < 0 || i2 >= int(vertices.size())
                ) {
                    continue;
                }

                scene.add_triangle(
                    Triangle(
                        vertices[i0],
                        vertices[i1],
                        vertices[i2],
                        material
                    )
                );

                triangle_count++;
            }
        }
    }

    std::cerr << "Loaded OBJ: " << filename << "\n";
    std::cerr << "Vertices: " << vertices.size() << "\n";
    std::cerr << "Triangles: " << triangle_count << "\n";

    return true;
}

#endif