#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <thread>
#include <functional>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "glm/glm.hpp"

#include "model.h"
#include "RenderObject.h"
#include "BVH.h"

int envmap_width, envmap_height;
std::vector<glm::vec3> envmap;

std::unique_ptr<AccelerationStructure> accelerationStructure = std::make_unique<BVH>(AccelerationStructure::DebugMode::Off);

struct Light {
    Light(const glm::vec3 &p, const float i) : position(p), intensity(i) {}
    glm::vec3 position;
    float intensity;
};

struct Material {
    Material(const float r, const glm::vec4 &a, const glm::vec3 &color, const float spec) : refractive_index(r), albedo(a), diffuse_color(color), specular_exponent(spec) {}
    Material() : refractive_index(1), albedo(1,0,0,0), diffuse_color(), specular_exponent() {}
    float refractive_index;
    glm::vec4 albedo;
    glm::vec3 diffuse_color;
    float specular_exponent;
};

struct Sphere {
    glm::vec3 center;
    float radius;
    Material material;

    Sphere(const glm::vec3 &c, const float r, const Material &m) : center(c), radius(r), material(m) {}

    bool ray_intersect(const glm::vec3& orig, const glm::vec3& dir, float& t0) const {
        glm::vec3 L = center - orig;
        float tca = glm::dot(L, dir);
        float d2 = glm::dot(L, L) - tca * tca;
        if (d2 > radius * radius) return false;
        float thc = sqrtf(radius * radius - d2);
        t0 = tca - thc;
        float t1 = tca + thc;
        if (t0 < 0) t0 = t1;
        if (t0 < 0) return false;
        return true;
    }
};

glm::vec3 reflect(const glm::vec3& I, const glm::vec3& N) {
    return I - N * 2.f * (glm::dot(I, N));
}

glm::vec3 refract(const glm::vec3& I, const glm::vec3& N, const float eta_t, const float eta_i = 1.f) { // Snell's law
    float cosi = -std::max(-1.f, std::min(1.f, glm::dot(I, N)));
    if (cosi < 0) return refract(I, -N, eta_i, eta_t); // if the ray comes from the inside the object, swap the air and the media
    float eta = eta_i / eta_t;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? glm::vec3(1, 0, 0) : I * eta + N * (eta * cosi - sqrtf(k)); // k<0 = total reflection, no ray to refract. I refract it anyways, this has no physical meaning
}

bool scene_intersect(const glm::vec3& orig, const glm::vec3& dir, const std::vector<Sphere>& spheres, std::vector<RenderObject>& renderObjects, glm::vec3& hit, glm::vec3& N, Material& material) {
    float primitive_dist = std::numeric_limits<float>::max();
    for (size_t i = 0; i < spheres.size(); i++) {
        float dist_intersect;
        if (spheres[i].ray_intersect(orig, dir, dist_intersect) && dist_intersect < primitive_dist) {
            primitive_dist = dist_intersect;
            hit = orig + dir * dist_intersect;
            N = glm::normalize(hit - spheres[i].center);
            material = spheres[i].material;
        }
    }

    for (size_t i = 0; i < renderObjects.size(); i++)
    {
        float dist_intersect = std::numeric_limits<float>::max();
        //if (renderObjects[i].intersect(orig, dir, dist_intersect))
        if (accelerationStructure->Traverse(orig, dir, dist_intersect))
        {
            if (dist_intersect < primitive_dist)
            {
                primitive_dist = dist_intersect;
                hit = orig + dir * dist_intersect;
                N = glm::normalize(hit - spheres[i].center);
                material = spheres[i].material;
            }
        }
    }

    float checkerboard_dist = std::numeric_limits<float>::max();
    if (fabs(dir.y) > 1e-3) {
        float d = -(orig.y + 4) / dir.y; // the checkerboard plane has equation y = -4
        glm::vec3 pt = orig + dir * d;
        if (d > 0 && fabs(pt.x) < 10 && pt.z<-10 && pt.z>-30 && d < primitive_dist) {
            checkerboard_dist = d;
            hit = pt;
            N = glm::vec3(0, 1, 0);
            material.diffuse_color = (int(.5 * hit.x + 1000) + int(.5 * hit.z)) & 1 ? glm::vec3(.3, .3, .3) : glm::vec3(.3, .2, .1);
        }
    }
    return std::min(primitive_dist, checkerboard_dist) < 1000;
}

glm::vec3 cast_ray(const glm::vec3& orig, const glm::vec3& dir, const std::vector<Sphere>& spheres, std::vector<RenderObject>& renderObjects, const std::vector<Light>& lights, size_t depth = 0) {
    glm::vec3 point, N;
    Material material;

    if (depth > 4 || !scene_intersect(orig, dir, spheres, renderObjects, point, N, material)) {
        return glm::vec3(0.2, 0.7, 0.8); // background color
    }

    glm::vec3 reflect_dir = glm::normalize(reflect(dir, N));
    glm::vec3 refract_dir = glm::normalize(refract(dir, N, material.refractive_index));
    glm::vec3 reflect_orig = glm::dot(reflect_dir, N) < 0 ? point - N * 1e-3f : point + N * 1e-3f; // offset the original point to avoid occlusion by the object itself
    glm::vec3 refract_orig = glm::dot(refract_dir, N) < 0 ? point - N * 1e-3f : point + N * 1e-3f;
    glm::vec3 reflect_color = cast_ray(reflect_orig, reflect_dir, spheres, renderObjects, lights, depth + 1);
    glm::vec3 refract_color = cast_ray(refract_orig, refract_dir, spheres, renderObjects, lights, depth + 1);

    float diffuse_light_intensity = 0, specular_light_intensity = 0;
    for (size_t i = 0; i < lights.size(); i++) {
        glm::vec3 light_dir = glm::normalize(lights[i].position - point);
        float light_distance = glm::length((lights[i].position - point));

        glm::vec3 shadow_orig = glm::dot(light_dir, N) < 0 ? point - N * 1e-3f : point + N * 1e-3f; // checking if the point lies in the shadow of the lights[i]
        glm::vec3 shadow_pt, shadow_N;
        Material tmpmaterial;
        if (scene_intersect(shadow_orig, light_dir, spheres, renderObjects, shadow_pt, shadow_N, tmpmaterial) && glm::length(shadow_pt - shadow_orig) < light_distance)
            continue;

        diffuse_light_intensity += lights[i].intensity * std::max(0.f, glm::dot(light_dir, N));
        specular_light_intensity += powf(std::max(0.f, glm::dot(-reflect(-light_dir, N), dir)), material.specular_exponent) * lights[i].intensity;
    }
    return material.diffuse_color * diffuse_light_intensity * material.albedo[0] + glm::vec3(1., 1., 1.) * specular_light_intensity * material.albedo[1] + reflect_color * material.albedo[2] + refract_color * material.albedo[3];
}

void render(const std::vector<Sphere> &spheres, std::vector<RenderObject> &renderObjects, const std::vector<Light> &lights) {
    const int   width    = 1024;
    const int   height   = 768;
    const float fov      = M_PI/3.;
    std::vector<glm::vec3> framebuffer(width*height);
    std::vector<std::thread> threadPool;
    threadPool.reserve(height);

    std::function<void(int)> renderThread = [&](int row) {
        for (size_t i = 0; i < width; i++) {
            float dir_x = (i + 0.5) - width / 2.;
            float dir_y = -(row + 0.5) + height / 2.;    // this flips the image at the same time
            float dir_z = -height / (2. * tan(fov / 2.));
            framebuffer[i + row * width] = cast_ray(glm::vec3(0, 0, 0), glm::normalize(glm::vec3(dir_x, dir_y, dir_z)), spheres, renderObjects, lights);
        }
    };

    for (size_t j = 0; j<height; j++) { // actual rendering loop
        threadPool.emplace_back(std::thread(renderThread, j));
    }

    for (int i = 0; i < height; i++) {
        threadPool[i].join();
    }

    std::vector<unsigned char> pixmap(width*height*3);
    for (size_t i = 0; i < height*width; ++i) {
        glm::vec3 &c = framebuffer[i];
        float max = std::max(c[0], std::max(c[1], c[2]));
        if (max>1) c = c*(1.f/max);
        for (size_t j = 0; j<3; j++) {
            pixmap[i*3+j] = (unsigned char)(255 * std::max(0.f, std::min(1.f, framebuffer[i][j])));
        }
    }
    stbi_write_jpg("out.jpg", width, height, 3, pixmap.data(), 100);
}

int main() {
    int n = -1;
    unsigned char *pixmap = stbi_load("../envmap.jpg", &envmap_width, &envmap_height, &n, 0);
    if (!pixmap || 3!=n) {
        std::cerr << "Error: can not load the environment map" << std::endl;
        return -1;
    }
    envmap = std::vector<glm::vec3>(envmap_width*envmap_height);
    for (int j = envmap_height-1; j>=0 ; j--) {
        for (int i = 0; i<envmap_width; i++) {
            envmap[i+j*envmap_width] = glm::vec3(pixmap[(i+j*envmap_width)*3+0], pixmap[(i+j*envmap_width)*3+1], pixmap[(i+j*envmap_width)*3+2])*(1/255.f);
        }
    }
    stbi_image_free(pixmap);

    Material      ivory(1.0, glm::vec4(0.6,  0.3, 0.1, 0.0), glm::vec3(0.4, 0.4, 0.3),   50.);
    Material      glass(1.5, glm::vec4(0.0,  0.5, 0.1, 0.8), glm::vec3(0.6, 0.7, 0.8),  125.);
    Material red_rubber(1.0, glm::vec4(0.9,  0.1, 0.0, 0.0), glm::vec3(0.3, 0.1, 0.1),   10.);
    Material     mirror(1.0, glm::vec4(0.0, 10.0, 0.8, 0.0), glm::vec3(1.0, 1.0, 1.0), 1425.);

    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(glm::vec3(-3,    0,   -16), 2,      ivory));
    spheres.push_back(Sphere(glm::vec3(-1.0, -1.5, -12), 2,      glass));
    spheres.push_back(Sphere(glm::vec3( 1.5, -0.5, -18), 3, red_rubber));
    spheres.push_back(Sphere(glm::vec3( 7,    5,   -18), 4,     mirror));

    std::vector<RenderObject> renderObjects;
    RenderObject duck(Model("../duck.obj"), Transform(glm::vec3(0, 0, -10), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1)));
    renderObjects.push_back(duck);
    accelerationStructure->AddObject(duck);
    std::cout << "Duck Added" << std::endl;
    accelerationStructure->Build(false);
    std::cout << "BVH Built" << std::endl;

    std::vector<Light>  lights;
    lights.push_back(Light(glm::vec3(-20, 20,  20), 1.5));
    lights.push_back(Light(glm::vec3( 30, 50, -25), 1.8));
    lights.push_back(Light(glm::vec3( 30, 20,  30), 1.7));

    render(spheres, renderObjects, lights);

    return 0;
}

