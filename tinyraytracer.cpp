#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <thread>
#include <functional>
#include <random>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "glm/glm.hpp"

#include "model.h"
#include "RenderObject.h"
#include "BVH.h"
#include "Ray.h"
#include "Material.h"

using AccelStruct = std::unique_ptr<AccelerationStructure>;
using RandomGenerator = std::default_random_engine;
using LightDistribution = std::uniform_int_distribution<int>;
using FloatDistribution = std::uniform_real_distribution<float>;

int envmap_width, envmap_height;
std::vector<glm::vec3> envmap;

struct Light {
    Light(const glm::vec3& p, const float i) : position(p), intensity(i), color(glm::vec3(0.8, 0.2, 0.2) * intensity) {}
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
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

uint32_t PCGHash(uint32_t& seed)
{
    uint32_t state = seed * 747796405u + 2891336453u;
    uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float RandomFloat(uint32_t& seed)
{
    return float(PCGHash(seed)) / float(std::numeric_limits<uint32_t>().max());
}

glm::vec3 UnitSphereVector(uint32_t& seed)
{
    glm::vec3 unitsphereVector = glm::vec3(RandomFloat(seed) - 0.5f, RandomFloat(seed) - 0.5f, RandomFloat(seed) - 0.5f);
    return glm::normalize(unitsphereVector);
}

glm::vec3 RandomHemisphereDirection(const glm::vec3& normal, RandomGenerator generator, FloatDistribution floatDistribution)
{
    uint32_t seed = static_cast<uint32_t>(floatDistribution(generator) * std::numeric_limits<uint32_t>().max());
    glm::vec3 randomDirection = UnitSphereVector(seed);
    int attempt = 0;

    while (glm::dot(randomDirection, normal) <= 0 && attempt < 40)
    {
        randomDirection = UnitSphereVector(seed);
        attempt++;
    }

    return randomDirection;
}

glm::vec3 TraceShadowRay(const Ray& ray, const AccelStruct& accelerationStructure, const std::vector<Light> lights, RandomGenerator generator, LightDistribution lightIndexDistribution)
{
    Light randomLight = lights[lightIndexDistribution(generator)];
    glm::vec3 lightPosition = randomLight.position;
    glm::vec3 lightDirection = lightPosition - ray.hitLocation;
    Ray shadowRay = Ray(ray.hitLocation, lightDirection);

    //Check if light shines towards ray.hitLocation surface
    float lightDistance = glm::length(lightDirection);
    lightDirection /= lightDistance; //normalisation
    float cosO = glm::dot(-lightDirection, -lightDirection); //TODO: add light normal support for area lights and emmisive objects
    float cosI = glm::dot(lightDirection, ray.normal);
    if (cosO <= 0 || cosI <= 0)
        return glm::vec3(0);

    //Check if light is blocked by object
    if (!accelerationStructure->Traverse(shadowRay))
        return glm::vec3(0);

    //Light contribution to surface
    glm::vec3 BRDF = ray.material.diffuseColor / std::_Pi;
    float solidAngle = (0.00000001f * cosO) / (lightDistance / lightDistance); //TODO: Add Area to light sources
    return BRDF * randomLight.color * solidAngle * cosI;
}

glm::vec3 PathTraceRay(Ray& ray, const AccelStruct& accelerationStructure, const std::vector<Light> lights, uint32_t rayDepth = 0, int maxRayDepth = 8)
{
    //// Old tinyRaytracer rendering code
    //if (rayDepth > maxRayDepth || !accelerationStructure->Traverse(ray))
    //    return glm::vec3(0.2, 0.7, 0.8); // background color
    //
    //glm::vec3 reflect_dir = glm::normalize(reflect(ray.direction, ray.normal));
    //glm::vec3 refract_dir = glm::normalize(refract(ray.direction, ray.normal, ray.material.refractiveIndex));
    //glm::vec3 reflect_orig = glm::dot(reflect_dir, ray.normal) < 0 ? ray.hitLocation - ray.normal * 1e-3f : ray.hitLocation + ray.normal * 1e-3f; // offset the original point to avoid occlusion by the object itself
    //glm::vec3 refract_orig = glm::dot(refract_dir, ray.normal) < 0 ? ray.hitLocation - ray.normal * 1e-3f : ray.hitLocation + ray.normal * 1e-3f;
    //glm::vec3 reflect_color = PathTraceRay(Ray(reflect_orig, reflect_dir), accelerationStructure, lights, rayDepth + 1);
    //glm::vec3 refract_color = PathTraceRay(Ray(refract_orig, refract_dir), accelerationStructure, lights, rayDepth + 1);
    //
    //float diffuse_light_intensity = 0, specular_light_intensity = 0;
    //for (size_t i = 0; i < lights.size(); i++) {
    //    glm::vec3 light_dir = glm::normalize(lights[i].position - ray.hitLocation);
    //    float light_distance = glm::length((lights[i].position - ray.hitLocation));
    //
    //    glm::vec3 shadow_orig = glm::dot(light_dir, ray.normal) < 0 ? ray.hitLocation - ray.normal * 1e-3f : ray.hitLocation + ray.normal * 1e-3f; // checking if the point lies in the shadow of the lights[i]
    //    glm::vec3 shadow_pt, shadow_N;
    //    Material tmpmaterial;
    //    Ray shadowRay(shadow_orig, light_dir);
    //    if (accelerationStructure->Traverse(shadowRay) && glm::length(shadowRay.hitLocation - shadowRay.origin) < light_distance)
    //        continue;
    //
    //    diffuse_light_intensity += lights[i].intensity * std::max(0.f, glm::dot(light_dir, ray.normal));
    //    specular_light_intensity += powf(std::max(0.f, glm::dot(-reflect(-light_dir, ray.normal), ray.direction)), ray.material.specularExponent) * lights[i].intensity;
    //}
    //
    //return ray.material.diffuseColor * diffuse_light_intensity * ray.material.albedo_old[0] + glm::vec3(1., 1., 1.) * specular_light_intensity * ray.material.albedo_old[1] + reflect_color * ray.material.albedo_old[2] + refract_color * ray.material.albedo_old[3];

    // Path tracing
    if (rayDepth > maxRayDepth || !accelerationStructure->Traverse(ray))
        return glm::vec3(0.2, 0.7, 0.8); // background color

    std::default_random_engine generator;
    FloatDistribution floatDistribution(0.0f, 1.0f);
    LightDistribution lightIndexDistribution(0, lights.size() - 1);

    //TODO: Add emmisive objects
    
    //Shadowray
    glm::vec3 shadowRayColor = TraceShadowRay(ray, accelerationStructure, lights, generator, lightIndexDistribution);

    // Hitlocation contribution
    // Material type cases
    Ray nextRay;
    switch (ray.material.type)
    {
        case Material::Type::Emmisive:
            return ray.material.emittance;
        case Material::Type::Mirror:
            nextRay = Ray(ray.hitLocation, reflect(ray.direction, ray.normal));
            return ray.material.albedo * PathTraceRay(nextRay, accelerationStructure, lights, rayDepth, maxRayDepth);
        //case Material::Type::Dielectric: // Glass
        //    Ray reflectionRay = Ray(ray.hitLocation, reflect(ray.direction, ray.normal));
        //    //TODO: handle Fresnel proportion based choice between reflecting and transmitting light
        //    break;
    }

    //reflection ray
    glm::vec3 reflectionDirection = RandomHemisphereDirection(ray.normal, generator, floatDistribution);
    nextRay = Ray(ray.hitLocation, reflectionDirection);

    //update throughput
    glm::vec3 BRDF = ray.material.diffuseColor / std::_Pi;
    float cosI = glm::dot(reflectionDirection, ray.normal);

    return 2.0f * std::_Pi * BRDF * cosI * PathTraceRay(nextRay, accelerationStructure, lights, rayDepth + 1, maxRayDepth);
}

void render(AccelStruct& accelerationStructure, const std::vector<Light> &lights) {
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
            framebuffer[i + row * width] = PathTraceRay(Ray(glm::vec3(0, 0, 0), glm::normalize(glm::vec3(dir_x, dir_y, dir_z))), accelerationStructure, lights);
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

    Material      ivory(Material::Type::Dielectric, glm::vec4(0.6,  0.3, 0.1, 0.0), glm::vec3(0.4, 0.4, 0.3), 1.0,   50.);
    Material      glass(Material::Type::Dielectric, glm::vec4(0.0,  0.5, 0.1, 0.8), glm::vec3(0.6, 0.7, 0.8), 1.5,  125.);
    Material red_rubber(Material::Type::Dielectric, glm::vec4(0.9,  0.1, 0.0, 0.0), glm::vec3(0.3, 0.1, 0.1), 1.0,   10.);
    Material     mirror(Material::Type::Mirror, glm::vec4(0.0, 10.0, 0.8, 0.0), glm::vec3(1.0, 1.0, 1.0), 1.0, 1425.);
    Material   emmisive(Material::Type::Emmisive, glm::vec4(0.0, 10.0, 0.8, 0.0), glm::vec3(1.0, 1.0, 1.0), 1.0, 1425.);
    emmisive.emittance = glm::vec3(1.0, 1.0, 1.0);

    std::vector<Material> materials;
    materials.reserve(4);
    materials.push_back(ivory);
    materials.push_back(glass);
    materials.push_back(red_rubber);
    materials.push_back(mirror);
    //materials.push_back(emmisive);

    AccelStruct accelerationStructure = std::make_unique<BVH>(AccelerationStructure::DebugMode::Off);

    std::vector<RenderObject> renderObjects;
    Model duckModel = Model("../duck.obj");
    RenderObject duck = RenderObject(duckModel, Material(), Transform(glm::vec3(1), glm::vec3(1), glm::vec3(1)));
    for (int i = 0; i < 10; i++)
    {
        Transform transform = Transform(glm::vec3(-22 + 5 * i, -23 + i * 5, -40), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
        duck = RenderObject(duckModel, materials[i % 4], transform);
        accelerationStructure->AddObject(duck);
        std::cout << "Duck Added" << std::endl;
    }
    accelerationStructure->Build(true);
    std::cout << "BVH Built" << std::endl;


    std::vector<Light>  lights;
    lights.push_back(Light(glm::vec3(-20, 20,  20), 1.5));
    lights.push_back(Light(glm::vec3( 30, 50, -25), 1.8));
    lights.push_back(Light(glm::vec3( 30, 20,  30), 1.7));

    render(accelerationStructure, lights);

    return 0;
}

