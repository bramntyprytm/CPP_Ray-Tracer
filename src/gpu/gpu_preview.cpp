#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include <sstream>
#include <iomanip>
#include <cmath>
#include <iostream>

#include "camera.h"
#include "obj_loader.h"
#include "scene.h"
#include "vec3.h"

bool first_mouse = true;
double last_mouse_x = 0.0;
double last_mouse_y = 0.0;
double mouse_delta_x = 0.0;
double mouse_delta_y = 0.0;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char info_log[2048];
        glGetShaderInfoLog(shader, 2048, nullptr, info_log);
        std::cerr << "Shader compilation failed:\n" << info_log << "\n";
    }

    return shader;
}

GLuint create_shader_program() {
    const char* vertex_shader_source = R"(
        #version 410 core

        layout(location = 0) in vec2 a_position;

        out vec2 v_uv;

        void main() {
            v_uv = a_position * 0.5 + 0.5;
            gl_Position = vec4(a_position, 0.0, 1.0);
        }
    )";

    const char* fragment_shader_source = R"(
        #version 410 core

        in vec2 v_uv;
        out vec4 frag_colour;

        uniform vec2 u_resolution;

        uniform vec3 u_camera_position;
        uniform vec3 u_camera_forward;
        uniform vec3 u_camera_right;
        uniform vec3 u_camera_up;
        uniform float u_fov_degrees;

        uniform samplerBuffer u_triangles;
        uniform int u_triangle_count;

        struct Ray {
            vec3 origin;
            vec3 direction;
        };

        struct Hit {
            float t;
            vec3 point;
            vec3 normal;
            bool hit;
            bool ground;
        };

        vec3 get_triangle_vertex(int triangle_index, int vertex_index) {
            int texel_index = triangle_index * 3 + vertex_index;
            return texelFetch(u_triangles, texel_index).xyz;
        }

        bool hit_triangle(
            Ray ray,
            vec3 v0,
            vec3 v1,
            vec3 v2,
            out float t,
            out vec3 normal
        ) {
            const float EPSILON = 0.0000001;

            vec3 edge1 = v1 - v0;
            vec3 edge2 = v2 - v0;

            vec3 h = cross(ray.direction, edge2);
            float a = dot(edge1, h);

            if (abs(a) < EPSILON) {
                return false;
            }

            float f = 1.0 / a;
            vec3 s = ray.origin - v0;
            float u = f * dot(s, h);

            if (u < 0.0 || u > 1.0) {
                return false;
            }

            vec3 q = cross(s, edge1);
            float v = f * dot(ray.direction, q);

            if (v < 0.0 || u + v > 1.0) {
                return false;
            }

            t = f * dot(edge2, q);

            if (t < 0.001) {
                return false;
            }

            normal = normalize(cross(edge1, edge2));

            if (dot(normal, ray.direction) > 0.0) {
                normal = -normal;
            }

            return true;
        }

        float hit_sphere(Ray ray, vec3 center, float radius) {
            vec3 oc = ray.origin - center;

            float a = dot(ray.direction, ray.direction);
            float b = 2.0 * dot(oc, ray.direction);
            float c = dot(oc, oc) - radius * radius;

            float discriminant = b * b - 4.0 * a * c;

            if (discriminant < 0.0) {
                return -1.0;
            }

            float sqrt_d = sqrt(discriminant);

            float t1 = (-b - sqrt_d) / (2.0 * a);
            float t2 = (-b + sqrt_d) / (2.0 * a);

            if (t1 > 0.001) return t1;
            if (t2 > 0.001) return t2;

            return -1.0;
        }

        Hit scene_hit(Ray ray) {
            Hit closest;
            closest.t = 1e30;
            closest.point = vec3(0.0);
            closest.normal = vec3(0.0);
            closest.hit = false;
            closest.ground = false;

            // Cow triangles
            for (int i = 0; i < u_triangle_count; i++) {
                vec3 v0 = get_triangle_vertex(i, 0);
                vec3 v1 = get_triangle_vertex(i, 1);
                vec3 v2 = get_triangle_vertex(i, 2);

                float t;
                vec3 normal;

                if (hit_triangle(ray, v0, v1, v2, t, normal)) {
                    if (t < closest.t) {
                        closest.t = t;
                        closest.point = ray.origin + t * ray.direction;
                        closest.normal = normal;
                        closest.hit = true;
                        closest.ground = false;
                    }
                }
            }

            // Reflective ground sphere
            float ground_t = hit_sphere(
                ray,
                vec3(0.0, -100.5, -1.0),
                100.0
            );

            if (ground_t > 0.0 && ground_t < closest.t) {
                closest.t = ground_t;
                closest.point = ray.origin + ground_t * ray.direction;
                closest.normal = normalize(closest.point - vec3(0.0, -100.5, -1.0));
                closest.hit = true;
                closest.ground = true;
            }

            return closest;
        }

        vec3 background_colour(Ray ray) {
            vec3 unit_direction = normalize(ray.direction);
            float a = 0.5 * (unit_direction.y + 1.0);

            vec3 white = vec3(1.0);
            vec3 blue = vec3(0.5, 0.7, 1.0);

            return mix(white, blue, a);
        }

        vec3 local_shade(Ray ray, Hit hit) {
            vec3 light_position = vec3(2.0, 3.0, 1.0);
            vec3 light_direction = normalize(light_position - hit.point);
            vec3 view_direction = normalize(-ray.direction);

            vec3 base_colour;
            float ambient;
            float diffuse_strength;
            float specular_strength;
            float shininess;

            if (hit.ground) {
                base_colour = vec3(0.45, 0.45, 0.45);
                ambient = 0.1;
                diffuse_strength = 1.0;
                specular_strength = 0.25;
                shininess = 32.0;
            } else {
                // Shiny cow material
                base_colour = vec3(0.85, 0.82, 0.75);
                ambient = 0.12;
                diffuse_strength = 1.0;
                specular_strength = 1.2;
                shininess = 128.0;
            }

            float diffuse = max(dot(hit.normal, light_direction), 0.0);

            Ray shadow_ray;
            shadow_ray.origin = hit.point + hit.normal * 0.001;
            shadow_ray.direction = light_direction;

            Hit shadow_hit = scene_hit(shadow_ray);
            float light_distance = length(light_position - hit.point);

            bool in_shadow = shadow_hit.hit && shadow_hit.t < light_distance;

            float specular = 0.0;

            if (!in_shadow) {
                vec3 halfway_direction = normalize(light_direction + view_direction);
                specular = max(dot(hit.normal, halfway_direction), 0.0);
                specular = pow(specular, shininess);
            }

            if (in_shadow) {
                diffuse = 0.0;
                specular = 0.0;
            }

            vec3 diffuse_colour =
                base_colour * (ambient + diffuse_strength * diffuse);

            vec3 specular_colour =
                vec3(1.0) * specular_strength * specular;

            return diffuse_colour + specular_colour;
        }

        float material_reflectivity(Hit hit) {
            if (hit.ground) {
                return 0.55;
            }

            // Reflective cow
            return 0.25;
        }

        vec3 trace_ray(Ray initial_ray) {
            vec3 final_colour = vec3(0.0);
            vec3 throughput = vec3(1.0);

            Ray ray = initial_ray;

            const int max_bounces = 3;

            for (int bounce = 0; bounce < max_bounces; bounce++) {
                Hit hit = scene_hit(ray);

                if (!hit.hit) {
                    final_colour += throughput * background_colour(ray);
                    break;
                }

                vec3 local_colour = local_shade(ray, hit);
                float reflectivity = material_reflectivity(hit);

                final_colour += throughput * local_colour * (1.0 - reflectivity);

                if (reflectivity <= 0.001) {
                    break;
                }

                throughput *= reflectivity;

                ray.origin = hit.point + hit.normal * 0.001;
                ray.direction = reflect(normalize(ray.direction), hit.normal);
            }

            return final_colour;
        }

        Ray generate_camera_ray(vec2 uv) {
            vec2 screen = uv * 2.0 - 1.0;
            screen.x *= u_resolution.x / u_resolution.y;

            float fov_rad = radians(u_fov_degrees);
            float scale = tan(fov_rad * 0.5);

            vec3 direction =
                normalize(
                    u_camera_forward
                    + screen.x * scale * u_camera_right
                    + screen.y * scale * u_camera_up
                );

            Ray ray;
            ray.origin = u_camera_position;
            ray.direction = direction;

            return ray;
        }

        void main() {
            Ray ray = generate_camera_ray(v_uv);
            vec3 colour = trace_ray(ray);

            colour = clamp(colour, vec3(0.0), vec3(1.0));
            colour = sqrt(colour);

            frag_colour = vec4(colour, 1.0);
        }
    )";

    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char info_log[2048];
        glGetProgramInfoLog(program, 2048, nullptr, info_log);
        std::cerr << "Shader program linking failed:\n" << info_log << "\n";
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

void upload_vec3(GLuint program, const char* name, const Vec3& value) {
    GLint location = glGetUniformLocation(program, name);
    glUniform3f(location, float(value.x), float(value.y), float(value.z));
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    (void)window;

    if (first_mouse) {
        last_mouse_x = xpos;
        last_mouse_y = ypos;
        first_mouse = false;
    }

    mouse_delta_x += xpos - last_mouse_x;
    mouse_delta_y += ypos - last_mouse_y;

    last_mouse_x = xpos;
    last_mouse_y = ypos;
}

GLuint create_triangle_texture_buffer(const Scene& scene, GLuint& triangle_buffer) {
    std::vector<float> data;

    for (const Triangle& tri : scene.triangles) {
        // v0
        data.push_back(float(tri.v0.x));
        data.push_back(float(tri.v0.y));
        data.push_back(float(tri.v0.z));
        data.push_back(0.0f);

        // v1
        data.push_back(float(tri.v1.x));
        data.push_back(float(tri.v1.y));
        data.push_back(float(tri.v1.z));
        data.push_back(0.0f);

        // v2
        data.push_back(float(tri.v2.x));
        data.push_back(float(tri.v2.y));
        data.push_back(float(tri.v2.z));
        data.push_back(0.0f);
    }

    glGenBuffers(1, &triangle_buffer);
    glBindBuffer(GL_TEXTURE_BUFFER, triangle_buffer);
    glBufferData(
        GL_TEXTURE_BUFFER,
        data.size() * sizeof(float),
        data.data(),
        GL_STATIC_DRAW
    );

    GLuint triangle_texture = 0;
    glGenTextures(1, &triangle_texture);
    glBindTexture(GL_TEXTURE_BUFFER, triangle_texture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, triangle_buffer);

    return triangle_texture;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialise GLFW\n";
        return 1;
    }

#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    const int window_width = 1280;
    const int window_height = 720;

    GLFWwindow* window = glfwCreateWindow(
        window_width,
        window_height,
        "GPU Ray Tracer Preview - Fragment Shader",
        nullptr,
        nullptr
    );

    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1);

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << "\n";
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "Controls: W/S/A/D move, Q/E down/up, mouse look, Esc quit\n";

    GLuint shader_program = create_shader_program();

    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,

        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    GLuint vao = 0;
    GLuint vbo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        reinterpret_cast<void*>(0)
    );
    glEnableVertexAttribArray(0);

    Camera camera;
    camera.position = Vec3(0, 0, 0);
    camera.yaw = -90.0;
    camera.pitch = 0.0;
    camera.fov_degrees = 60.0;
    camera.aspect_ratio = double(window_width) / double(window_height);

    double previous_time = glfwGetTime();

    double fps_timer = 0.0;
    int frame_count = 0;
    double current_fps = 0.0;

    Scene scene;

    Material cow_material(
        Vec3(0.8, 0.8, 0.8),
        0.1,
        1.0,
        1.0,
        96.0,
        0.35
    );

    load_obj_as_triangles(
        "../models/cow.obj",
        scene,
        cow_material,
        0.4,
        Vec3(0.0, -0.5, -2.5)
    );

    std::cout << "GPU preview cow triangles: "
            << scene.triangles.size()
            << "\n";

    GLuint triangle_buffer = 0;
    GLuint triangle_texture = create_triangle_texture_buffer(scene, triangle_buffer);
    int triangle_count = int(scene.triangles.size());

    while (!glfwWindowShouldClose(window)) {
        double current_time = glfwGetTime();
        double delta_time = current_time - previous_time;
        previous_time = current_time;

        fps_timer += delta_time;
        frame_count++;

        if (fps_timer >= 0.5) {
            current_fps = double(frame_count) / fps_timer;

            std::ostringstream title;
            title << "GPU Ray Tracer Preview | FPS: "
                << std::fixed << std::setprecision(1) << current_fps
                << " | Pos: ("
                << std::setprecision(2)
                << camera.position.x << ", "
                << camera.position.y << ", "
                << camera.position.z << ")";

            glfwSetWindowTitle(window, title.str().c_str());

            fps_timer = 0.0;
            frame_count = 0;
        }

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        double move_speed = 2.0 * delta_time;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camera.position += camera.forward() * move_speed;
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camera.position += camera.forward() * -move_speed;
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            camera.position += camera.right() * -move_speed;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camera.position += camera.right() * move_speed;
        }

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            camera.position.y -= move_speed;
        }

        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            camera.position.y += move_speed;
        }

        double mouse_sensitivity = 0.08;

        camera.yaw += mouse_delta_x * mouse_sensitivity;
        camera.pitch -= mouse_delta_y * mouse_sensitivity;

        mouse_delta_x = 0.0;
        mouse_delta_y = 0.0;

        if (camera.pitch > 89.0) {
            camera.pitch = 89.0;
        }

        if (camera.pitch < -89.0) {
            camera.pitch = -89.0;
        }

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, triangle_texture);
        glUniform1i(glGetUniformLocation(shader_program, "u_triangles"), 0);

        glUniform1i(
            glGetUniformLocation(shader_program, "u_triangle_count"),
            triangle_count
        );

        glUniform2f(
            glGetUniformLocation(shader_program, "u_resolution"),
            float(window_width),
            float(window_height)
        );

        upload_vec3(shader_program, "u_camera_position", camera.position);
        upload_vec3(shader_program, "u_camera_forward", camera.forward());
        upload_vec3(shader_program, "u_camera_right", camera.right());
        upload_vec3(shader_program, "u_camera_up", camera.up());

        glUniform1f(
            glGetUniformLocation(shader_program, "u_fov_degrees"),
            float(camera.fov_degrees)
        );

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);

    glDeleteTextures(1, &triangle_texture);
    glDeleteBuffers(1, &triangle_buffer);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}