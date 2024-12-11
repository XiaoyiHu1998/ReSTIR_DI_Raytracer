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
#include "Utils.h"

using AccelStruct = std::unique_ptr<AccelerationStructure>;
using RandomGenerator = std::default_random_engine;
using LightDistribution = std::uniform_int_distribution<int>;
using FloatDistribution = std::uniform_real_distribution<float>;

float EPSILON = 0.00001f;

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

glm::vec3 UnitSphereVector(uint32_t& seed)
{
    glm::vec3 unitsphereVector = glm::vec3(Utils::RandomFloat(seed) - 0.5f, Utils::RandomFloat(seed) - 0.5f, Utils::RandomFloat(seed) - 0.5f);
    return glm::normalize(unitsphereVector);
}

glm::vec3 RandomHemisphereDirection(const glm::vec3& normal, uint32_t& seed)
{
    glm::vec3 randomDirection = UnitSphereVector(seed);
    uint32_t attempt = 0;

    while (glm::dot(randomDirection, normal) <= 0 && attempt < 200)
    {
        uint32_t attemptSeed = seed + attempt;
        randomDirection = UnitSphereVector(attemptSeed);
        attempt++;
    }

    return randomDirection;
}

glm::vec3 TraceDirectLight(const Ray& ray, const AccelStruct& accelerationStructure, const AccelStruct& lights, uint32_t& seed)
{
    RandomLightPoint lightPoint = lights->RandomTrianglePoint(seed);
    glm::vec3 lightPosition = lightPoint.point; // object space
    glm::vec3 lightPositionWorldSpace = glm::inverse(lightPoint.objectInverseTransform) * glm::vec4(lightPosition, 1.0f);
    glm::vec3 lightDirection = lightPositionWorldSpace - ray.hitLocation;
    glm::vec3 lightNormal = lightPoint.normal;
    float lightArea = lightPoint.area;
    float lightDistance = glm::length(lightDirection);
    lightDirection = glm::normalize(lightDirection);

    Ray shadowRay = Ray(ray.hitLocation + EPSILON * lightDirection, lightDirection, lightDistance - 2 * EPSILON);
    if (glm::dot(ray.normal, lightDirection) > 0 && glm::dot(lightNormal, -lightDirection) > 0)
    {
        //std::cout << "Lights pointing at eachother - Sending ShadowRay" << std::endl;
        bool shadowRayHit = accelerationStructure->Traverse(shadowRay);
        bool hitNonEmmisiveObject = shadowRayHit && shadowRay.material.type != Material::Type::Emmisive;

        if (!hitNonEmmisiveObject)
        {
            float solidAngle = (glm::dot(lightNormal, -lightDirection) * lightArea) / (lightDistance * lightDistance);
            return lightPoint.color * solidAngle * glm::dot(ray.normal, lightDirection);
        }
    }

    return glm::vec3(0);
}

glm::vec3 PathTraceRay(Ray& ray, bool prevHitMirrorDielectric, const AccelStruct& accelerationStructure, const AccelStruct& lights, uint32_t& seed, uint32_t rayDepth = 0, int maxRayDepth = 8)
{
    if (rayDepth > maxRayDepth || !accelerationStructure->Traverse(ray))
        return glm::vec3(0);

    //Debug renders
    //return glm::vec3(ray.normal.x * 0.5 + 0.5, ray.normal.y * 0.5 + 0.5, ray.normal.z * 0.5 + 0.5);
    //return TraceDirectLight(ray, accelerationStructure, lights);

    glm::vec3 BRDF = ray.material.albedo / std::_Pi;

    Ray nextRay;
    switch (ray.material.type)
    {
        case Material::Type::Emmisive:
            return (rayDepth == 0 || prevHitMirrorDielectric) ? ray.material.emittance : glm::vec3(0);
        case Material::Type::Mirror:
            nextRay = Ray(ray.hitLocation, reflect(ray.direction, ray.normal));
            return ray.material.albedo * PathTraceRay(nextRay, true, accelerationStructure, lights, seed, rayDepth + 1, maxRayDepth);
        //case Material::Type::Dielectric:
        //    return glm::vec3(0);
    }

    glm::vec3 directLighting = BRDF * TraceDirectLight(ray, accelerationStructure, lights, seed);

    glm::vec3 nextRayDirection = RandomHemisphereDirection(ray.normal, seed);
    nextRay = Ray(ray.hitLocation + EPSILON * nextRayDirection, nextRayDirection - 2 * EPSILON);
    glm::vec3 indirectLighting = glm::dot(ray.normal, nextRayDirection) * PathTraceRay(nextRay, false, accelerationStructure, lights, seed, rayDepth + 1, maxRayDepth);
    
    return std::_Pi * 2.0f * BRDF * indirectLighting + directLighting;
    //return std::_Pi * 2.0f * BRDF * indirectLighting;
    //return directLighting;
}

void render(AccelStruct& accelerationStructure, const AccelStruct& lights) {
    const int   width    = 1024;
    const int   height   = 768;
    const float fov      = M_PI/3.;
    std::vector<glm::vec3> framebuffer(width*height);
    std::vector<std::thread> threadPool;
    threadPool.reserve(height);

    std::function<void(int)> renderThread = [&](int row) {
        float sampleCount = 50.0f;
        std::default_random_engine generator;
        std::uniform_real_distribution<float> floatDistribution(0.0f, 0.999f);
        for (size_t i = 0; i < width; i++) {
            glm::vec3 framebufferPixelValue = glm::vec3(0);
            uint32_t seed = (row * width + i) * 10000;

            for (int sample = 0; sample < sampleCount; sample++)
            {
                float dir_x = (i + 0.5) - width / 2. + Utils::RandomFloat(seed) * 0.999f;
                float dir_y = -(row + 0.5) + height / 2. + Utils::RandomFloat(seed) * 0.999f;    // this flips the image at the same time
                float dir_z = -height / (2. * tan(fov / 2.)) + Utils::RandomFloat(seed) * 0.999f;

                framebufferPixelValue += PathTraceRay(Ray(glm::vec3(0, 0, 0), glm::normalize(glm::vec3(dir_x, dir_y, dir_z))), false, accelerationStructure, lights, seed);
                if (i == 978 && row == 210)
                {
                    std::cout << "color value " << i << "," << row << ": " << glm::to_string(framebufferPixelValue) << "| seed: " << seed << std::endl;
                }
                //framebufferPixelValue += PathTraceRay(Ray(glm::vec3(0, 0, 0), glm::normalize(glm::vec3(dir_x, dir_y, dir_z))), accelerationStructure, lights, seed) / sampleCount;
                seed++;
            }

            framebuffer[i + row * width] = (1.0f / sampleCount) * framebufferPixelValue;
            if (i == 978 && row == 210)
                std::cout << "color value " << i << "," << row << ": " << glm::to_string(framebuffer[i + row * width]) << std::endl;
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

    Material      ivory(Material::Type::Dielectric, glm::vec3(0.4, 0.4, 0.3), glm::vec3(0.4, 0.4, 0.3), 1.0,   50.);
    Material      glass(Material::Type::Dielectric, glm::vec3(0.6, 0.7, 0.8), glm::vec3(0.6, 0.7, 0.8), 1.5,  125.);
    Material red_rubber(Material::Type::Dielectric, glm::vec3(0.3, 0.1, 0.1), glm::vec3(0.3, 0.1, 0.1), 1.0,   10.);
    Material     mirror(Material::Type::Mirror, glm::vec3(0.8), glm::vec3(1.0, 1.0, 1.0), 1.0, 1425.);
    Material   emmisive(Material::Type::Emmisive, glm::vec3(1.0, 1.0, 1.0), glm::vec3(1.0, 1.0, 1.0), 1.0, 1425.);
    emmisive.emittance = glm::vec3(std::numeric_limits<float>().max(), std::numeric_limits<float>().max(), std::numeric_limits<float>().max());

    std::vector<Material> materials;
    materials.reserve(4);
    materials.push_back(ivory);
    materials.push_back(glass);
    materials.push_back(red_rubber);
    materials.push_back(mirror);
    materials.push_back(emmisive);

    AccelStruct accelerationStructure = std::make_unique<BVH>(AccelerationStructure::DebugMode::Off);
    AccelStruct lights = std::make_unique<BVH>(AccelerationStructure::DebugMode::Off);

    std::vector<RenderObject> renderObjects;
    Model duckModel = Model("../duck.obj");
    Model sphereModel = Model("../sphere.obj");
    //RenderObject duck = RenderObject(duckModel, Material(), Transform(glm::vec3(1), glm::vec3(1), glm::vec3(1)));
    //RenderObject sphere = RenderObject(duckModel, Material(), Transform(glm::vec3(1), glm::vec3(1), glm::vec3(1)));
    for (int i = 0; i < 10; i++)
    {
        Transform transform = Transform(glm::vec3(-22 + 5 * i, -23 + i * 5, -40), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
        RenderObject object = RenderObject(duckModel, materials[i % 5], transform);
        if (i % 5 == 4)
        {
            lights->AddObject(object);
        }
        accelerationStructure->AddObject(object);
        std::cout << "Duck Added" << std::endl;
    }
    accelerationStructure->Build(true);
    std::cout << "BVH Built" << std::endl;

    render(accelerationStructure, lights);

    return 0;
}

