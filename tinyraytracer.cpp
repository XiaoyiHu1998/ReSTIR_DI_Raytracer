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

#include "glm/glm.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "model.h"
#include "RenderObject.h"
#include "BVH.h"
#include "Octree.h"
#include "KdTree.h"
#include "Ray.h"
#include "Material.h"
#include "Utils.h"
#include "OutputImage.h"

using AccelStruct = std::unique_ptr<AccelerationStructure>;
using RandomGenerator = std::default_random_engine;
using LightDistribution = std::uniform_int_distribution<int>;
using FloatDistribution = std::uniform_real_distribution<float>;

float EPSILON = 0.00001f;

int envmap_width, envmap_height;
std::vector<glm::vec3> envmap;

struct DebugInfo
{
    long long int triangleIntersectionCount;
    long long int triangleIntersectionTestCount;
    long long int nodeIntersectionTestCount;
    long long int nodeIntersectionCount;
    long long int hitCount;

    DebugInfo()
    {
        triangleIntersectionCount = 0;
        triangleIntersectionTestCount = 0;
        nodeIntersectionTestCount = 0;
        nodeIntersectionCount = 0;
        hitCount = 0;
    }
};

void OutputImage(const std::vector<glm::vec3> framebuffer, const int width, const int height, std::string filename)
{
    std::vector<unsigned char> pixmap(width * height * 3);
    for (size_t i = 0; i < height * width; ++i)
    {
        glm::vec3 c = framebuffer[i];
        float max = std::max(c[0], std::max(c[1], c[2]));
        if (max > 1) c = c * (1.f / max);
        for (size_t j = 0; j < 3; j++)
        {
            pixmap[i * 3 + j] = (unsigned char)(255 * std::max(0.f, std::min(1.f, framebuffer[i][j])));
        }
    }
    stbi_write_jpg(filename.c_str(), width, height, 3, pixmap.data(), 100);
}

// adapted from https://axonflux.com/handy-rgb-to-hsl-and-rgb-to-hsv-color-model-c
glm::vec3 HsvToRgb(float h, float s, float v)
{
    float r = 0, g = 0, b = 0;

    int i = std::floor(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6)
    {
    case 0: r = v, g = t, b = p; break;
    case 1: r = q, g = v, b = p; break;
    case 2: r = p, g = v, b = t; break;
    case 3: r = p, g = q, b = v; break;
    case 4: r = t, g = p, b = v; break;
    case 5: r = v, g = p, b = q; break;
    }

    return glm::vec3(r, g, b);
}

float SchlicksApproximation(float refractionIndex, glm::vec3 incidence, glm::vec3 normal)
{
    float F0Top = (refractionIndex - 1) * (refractionIndex - 1);
    float F0Bottom = (refractionIndex + 1) * (refractionIndex + 1);
    float F0 = F0Top / F0Bottom;

    float lastTermInside = 1 - (glm::dot(normal, incidence));
    float lastTerm = lastTermInside * lastTermInside * lastTermInside * lastTermInside * lastTermInside;

    return F0 + (1 - F0) * lastTerm;
}

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
            return lightPoint.emittedRadiance * solidAngle * glm::dot(ray.normal, lightDirection);
        }
    }

    return glm::vec3(0);
}

glm::vec3 PathTraceRay(Ray& ray, bool prevHitMirrorDielectric, const AccelStruct& accelerationStructure, const AccelStruct& lights, uint32_t& seed, uint32_t rayDepth = 0, int maxRayDepth = 8)
{
    if (rayDepth > maxRayDepth || !accelerationStructure->Traverse(ray))
        return glm::vec3(0.2, 0.5, 0.7);

    //Debug renders
    //return glm::vec3(ray.normal.x * 0.5 + 0.5, ray.normal.y * 0.5 + 0.5, ray.normal.z * 0.5 + 0.5);
    //return TraceDirectLight(ray, accelerationStructure, lights);

    glm::vec3 BRDF = ray.material.albedo / std::_Pi;

    Ray nextRay;
    switch (ray.material.type)
    {
        case Material::Type::Emmisive:
            return (rayDepth == 0 || prevHitMirrorDielectric) ? ray.material.emittedRadiance : glm::vec3(0);
        case Material::Type::Mirror:
            nextRay = Ray(ray.hitLocation, reflect(ray.direction, ray.normal));
            return ray.material.albedo * PathTraceRay(nextRay, true, accelerationStructure, lights, seed, rayDepth + 1, maxRayDepth);
        case Material::Type::Dielectric:
            float fresnelReflectance = SchlicksApproximation(ray.material.refractiveIndex, ray.direction, ray.normal);
            float randomValue = Utils::RandomFloat(seed) * 0.9999f;

            //reflect
            if (randomValue < fresnelReflectance)
            {
                nextRay = Ray(ray.hitLocation, reflect(ray.direction, ray.normal));
                return ray.material.albedo * PathTraceRay(nextRay, true, accelerationStructure, lights, seed, rayDepth + 1, maxRayDepth);
            }
            //refract
            else 
            {
                float refractiveIndex1 = ray.material.refractiveIndex, refractiveIndex2 = ray.prevHitMaterial.refractiveIndex;
                glm::vec3 normal = ray.normal;
                
                bool exitingMedium = glm::dot(glm::normalize(ray.direction), glm::normalize(ray.normal)) < 0;
                if (exitingMedium)
                {
                    std::swap(refractiveIndex1, refractiveIndex2);
                    normal = -normal;
                }
                
                nextRay = Ray(ray.hitLocation, refract(ray.direction, ray.normal, ray.material.refractiveIndex));
                return ray.material.albedo * PathTraceRay(nextRay, true, accelerationStructure, lights, seed, rayDepth + 1, maxRayDepth);
            }   
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
    //std::vector<std::unique_ptr<Ray>> debugbuffer(width * height);
    std::vector<DebugInfo> debugbuffer(width * height);

    std::vector<std::thread> threadPool;
    threadPool.reserve(height);

    std::function<void(int)> renderThread = [&](int row) {
        float sampleCount = 10.0f;
        for (size_t i = 0; i < width; i++) {
            glm::vec3 framebufferPixelValue = glm::vec3(0);
            uint32_t seed = (row * width + i) * 10000;

            for (int sample = 0; sample < sampleCount; sample++)
            {
                float dir_x = (i + 0.5) - width / 2. + Utils::RandomFloat(seed) * 0.999f;
                float dir_y = -(row + 0.5) + height / 2. + Utils::RandomFloat(seed) * 0.999f;    // this flips the image at the same time
                float dir_z = -height / (2. * tan(fov / 2.)) + Utils::RandomFloat(seed) * 0.999f;

                Ray ray = Ray(glm::vec3(0, 0, 0), glm::normalize(glm::vec3(dir_x, dir_y, dir_z)));
                framebufferPixelValue += PathTraceRay(ray, false, accelerationStructure, lights, seed);
                seed++;

                debugbuffer[i + row * width].triangleIntersectionCount += ray.triangleIntersectionCount;
                debugbuffer[i + row * width].triangleIntersectionTestCount += ray.triangleIntersectionTestCount;
                debugbuffer[i + row * width].nodeIntersectionCount += ray.nodeIntersectionCount;
                debugbuffer[i + row * width].nodeIntersectionTestCount += ray.nodeIntersectionTestCount;
                debugbuffer[i + row * width].hitCount += ray.hitCount;
            }

            framebuffer[i + row * width] = (1.0f / sampleCount) * framebufferPixelValue;
        }
    };

    for (size_t j = 0; j<height; j++) { // actual rendering loop
        threadPool.emplace_back(std::thread(renderThread, j));
    }

    for (int i = 0; i < height; i++) {
        threadPool[i].join();
    }

    // save the output image to file
    OutputImage(framebuffer, width, height, "output.jpg");

    // create debug images to visualize the ray intersections, box intersections, and traversal steps
    std::vector<glm::vec3> triangleIntersectionCountImage(width * height);
    std::vector<glm::vec3> triangleIntersectionTestCountImage(width * height);
    std::vector<glm::vec3> nodeIntersectionCountImage(width * height);
    std::vector<glm::vec3> nodeIntersectionTestCountImage(width * height);
    std::vector<glm::vec3> hitCountImage(width * height);

    auto valueToDebugColor = [](float value, float minValue, float maxValue, float minHue = 0.46f, float maxHue = 0.f)
    {
        float clampedValue = std::max(minValue, std::min(maxValue, value));
        float hue = minHue + (maxHue - minHue) * (clampedValue - minValue) / (maxValue - minValue);
        return HsvToRgb(hue, 1.f, 1.f);
    };

    auto min = [](long long int left, long long int right)
    {
        return left < right ? left : right;
    };

    auto max = [](long long int left, long long int right)
    {
        return left > right ? left : right;
    };

    long long int minTriangleIntersectionCount = std::numeric_limits<long long int>().max();
    long long int minTriangleIntersectionTestCount = std::numeric_limits<long long int>().max();
    long long int minNodeIntersectionCount = std::numeric_limits<long long int>().max();
    long long int minNodeIntersectionTestCount = std::numeric_limits<long long int>().max();
    long long int minHitCount = std::numeric_limits<long long int>().max();

    long long int maxTriangleIntersectionCount = 0;
    long long int maxTriangleIntersectionTestCount = 0;
    long long int maxNodeIntersectionCount = 0;
    long long int maxNodeIntersectionTestCount = 0;
    long long int maxHitCount = 0;

    long long int TotalTriangleIntersectionCount = 0;
    long long int TotalTriangleIntersectionTestCount = 0;
    long long int TotalNodeIntersectionCount = 0;
    long long int TotalNodeIntersectionTestCount = 0;
    long long int TotalHitCount = 0;
    for (int i = 0; i < width * height; i++)
    {
        DebugInfo debugInfo = debugbuffer[i];

        minTriangleIntersectionCount        = min(minTriangleIntersectionCount, debugInfo.triangleIntersectionCount);
        minTriangleIntersectionTestCount    = min(minTriangleIntersectionTestCount, debugInfo.triangleIntersectionTestCount);
        minNodeIntersectionCount            = min(minNodeIntersectionCount, debugInfo.nodeIntersectionCount);
        minNodeIntersectionTestCount        = min(minNodeIntersectionTestCount, debugInfo.nodeIntersectionTestCount);
        minHitCount                         = min(minHitCount, debugInfo.hitCount);

        maxTriangleIntersectionCount        = max(maxTriangleIntersectionCount, debugInfo.triangleIntersectionCount);
        maxTriangleIntersectionTestCount    = max(maxTriangleIntersectionTestCount, debugInfo.triangleIntersectionTestCount);
        maxNodeIntersectionCount            = max(maxNodeIntersectionCount, debugInfo.nodeIntersectionCount);
        maxNodeIntersectionTestCount        = max(maxNodeIntersectionTestCount, debugInfo.nodeIntersectionTestCount);
        maxHitCount                         = max(maxHitCount, debugInfo.hitCount);

        TotalTriangleIntersectionCount      += debugInfo.triangleIntersectionCount;
        TotalTriangleIntersectionTestCount  += debugInfo.triangleIntersectionTestCount;
        TotalNodeIntersectionCount          += debugInfo.nodeIntersectionCount;
        TotalNodeIntersectionTestCount      += debugInfo.nodeIntersectionTestCount;
        TotalHitCount                       += debugInfo.hitCount;
    }

    std::cout << "MinTriangleIntersectionCount: "       << minTriangleIntersectionCount << std::endl;
    std::cout << "MinTriangleIntersectionTestCount: "   << minTriangleIntersectionTestCount << std::endl;
    std::cout << "MinNodeIntersectionCount: "           << minNodeIntersectionCount << std::endl;
    std::cout << "MinNodeIntersectionTestCount: "       << minNodeIntersectionTestCount << std::endl;
    std::cout << "MinHitCount: "                        << minHitCount << std::endl;

    std::cout << "MaxTriangleIntersectionCount: "       << maxTriangleIntersectionCount << std::endl;
    std::cout << "MaxTriangleIntersectionTestCount: "   << maxTriangleIntersectionTestCount << std::endl;
    std::cout << "MaxNodeIntersectionCount: "           << maxNodeIntersectionCount << std::endl;
    std::cout << "MaxNodeIntersectionTestCount: "       << maxNodeIntersectionTestCount << std::endl;
    std::cout << "MaxHitCount: "                        << maxHitCount << std::endl;

    std::cout << "TotalTriangleIntersectionCount: "     << TotalTriangleIntersectionCount << std::endl;
    std::cout << "TotalTriangleIntersectionTestCount: " << TotalTriangleIntersectionTestCount << std::endl;
    std::cout << "TotalNodeIntersectionCount: "         << TotalNodeIntersectionCount << std::endl;
    std::cout << "TotalNodeIntersectionTestCount: "     << TotalNodeIntersectionTestCount << std::endl;
    std::cout << "TotalHitCount: "                      << TotalHitCount << std::endl;

    for (int i = 0; i < width * height; i++)
    {
        triangleIntersectionCountImage[i]       = valueToDebugColor(debugbuffer[i].triangleIntersectionCount, 0, maxTriangleIntersectionCount);
        triangleIntersectionTestCountImage[i]   = valueToDebugColor(debugbuffer[i].triangleIntersectionTestCount, 0, maxNodeIntersectionTestCount);
        nodeIntersectionCountImage[i]           = valueToDebugColor(debugbuffer[i].nodeIntersectionCount, 0, maxNodeIntersectionCount);
        nodeIntersectionTestCountImage[i]       = valueToDebugColor(debugbuffer[i].nodeIntersectionTestCount, 0, maxNodeIntersectionTestCount);
        hitCountImage[i]                        = valueToDebugColor(debugbuffer[i].hitCount, 0, maxHitCount);
    }

    OutputImage(triangleIntersectionCountImage, width, height, "triangleIntersectionCount.jpg");
    OutputImage(triangleIntersectionTestCountImage, width, height, "triangleIntersectionTestCount.jpg");
    OutputImage(nodeIntersectionCountImage, width, height, "nodeIntersectionCount.jpg");
    OutputImage(nodeIntersectionTestCountImage, width, height, "nodeIntersectionTestCount.jpg");
    OutputImage(hitCountImage, width, height, "nodeHitCount.jpg");
}

// high poly ducks stacked in front of eachother
void GenerateScene1(AccelStruct& accelerationStructure, AccelStruct& lights)
{
    Material      ivory(Material::Type::Dielectric, glm::vec3(0.4, 0.4, 0.3), glm::vec3(0.4, 0.4, 0.3), 1.0, 50.);
    Material      glass(Material::Type::Dielectric, glm::vec3(0.6, 0.7, 0.8), glm::vec3(0.6, 0.7, 0.8), 1.5, 125.);
    Material red_rubber(Material::Type::Dielectric, glm::vec3(0.3, 0.1, 0.1), glm::vec3(0.3, 0.1, 0.1), 1.0, 10.);
    Material     mirror(Material::Type::Mirror, glm::vec3(0.8), glm::vec3(1.0, 1.0, 1.0), 1.0, 1425.);
    Material   emissiveDuck(Material::Type::Emmisive, glm::vec3(1.0, 1.0, 1.0), glm::vec3(1.0, 1.0, 1.0), 1.0, 1425.);
    Material   ceiling(Material::Type::Emmisive, glm::vec3(1.0, 1.0, 1.0), glm::vec3(1.0, 1.0, 1.0), 1.0, 1425.);
    emissiveDuck.emittedRadiance = glm::vec3(1.0f, 1.0f, 1.0f);
    ceiling.emittedRadiance = glm::vec3(1.0f, 1.0f, 1.0f);

    std::vector<Material> materials;
    materials.reserve(4);
    materials.push_back(ivory);
    materials.push_back(glass);
    materials.push_back(red_rubber);
    materials.push_back(mirror);
    materials.push_back(emissiveDuck);

    std::vector<RenderObject> renderObjects;
    Model duckModel = Model("../duck.obj");
    Model duck1Model = Model("../duck1.obj");
    Model duck2Model = Model("../duck2.obj");
    Model duckSubdividedModel = Model("../duck_subdivided.obj");
    Model sphereModel = Model("../sphere.obj");
    Model planeModel = Model("../plane.obj");
    Transform transform = Transform(glm::vec3(0), glm::vec3(0), glm::vec3(0));
    for (int i = 0; i < 10; i++)
    {
        transform = Transform(glm::vec3(-20 + 2.0f * i, -9, -30 + 2 * i), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
        RenderObject object = RenderObject(duckSubdividedModel, materials[i % 5], transform);
        if (i % 5 == 4)
        {
            lights->AddObject(object);
        }
        accelerationStructure->AddObject(object);
        std::cout << "Object Added" << std::endl;
    }
    transform.scale = glm::vec3(40, 40, 40);
    RenderObject ceilingObject = RenderObject(planeModel, ceiling, Transform(glm::vec3(0, 10, -40), glm::vec3(0, 0, 0), glm::vec3(40, 40, 40)));
    RenderObject bigDuck = RenderObject(duckSubdividedModel, red_rubber, Transform(glm::vec3(0, -20, -40), glm::vec3(0, 0, 0), glm::vec3(40, 40, 40)));
    RenderObject floorObject = RenderObject(planeModel, ivory, Transform(glm::vec3(0, -20, -40), glm::vec3(0, 0, 0), glm::vec3(40, 40, 40)));

    lights->AddObject(ceilingObject);
    accelerationStructure->AddObject(ceilingObject);
    accelerationStructure->AddObject(bigDuck);
    accelerationStructure->AddObject(floorObject);
}

// high poly ducks next to each other
void GenerateScene2(AccelStruct& accelerationStructure, AccelStruct& lights)
{
    Material      ivory(Material::Type::Dielectric, glm::vec3(0.4, 0.4, 0.3), glm::vec3(0.4, 0.4, 0.3), 1.0, 50.);
    Material      glass(Material::Type::Dielectric, glm::vec3(0.6, 0.7, 0.8), glm::vec3(0.6, 0.7, 0.8), 1.5, 125.);
    Material red_rubber(Material::Type::Dielectric, glm::vec3(0.3, 0.1, 0.1), glm::vec3(0.3, 0.1, 0.1), 1.0, 10.);
    Material     mirror(Material::Type::Mirror, glm::vec3(0.8), glm::vec3(1.0, 1.0, 1.0), 1.0, 1425.);
    Material   emissiveDuck(Material::Type::Emmisive, glm::vec3(1.0, 1.0, 1.0), glm::vec3(1.0, 1.0, 1.0), 1.0, 1425.);
    Material   ceiling(Material::Type::Emmisive, glm::vec3(1.0, 1.0, 1.0), glm::vec3(1.0, 1.0, 1.0), 1.0, 1425.);
    emissiveDuck.emittedRadiance = glm::vec3(1.0f, 1.0f, 1.0f);
    ceiling.emittedRadiance = glm::vec3(1.0f, 1.0f, 1.0f);

    std::vector<Material> materials;
    materials.reserve(4);
    materials.push_back(ivory);
    materials.push_back(glass);
    materials.push_back(red_rubber);
    materials.push_back(mirror);
    materials.push_back(emissiveDuck);

    std::vector<RenderObject> renderObjects;
    Model duckModel = Model("../duck.obj");
    Model duck1Model = Model("../duck1.obj");
    Model duck2Model = Model("../duck2.obj");
    Model duckSubdividedModel = Model("../duck_subdivided.obj");
    Model sphereModel = Model("../sphere.obj");
    Model planeModel = Model("../plane.obj");
    Transform transform = Transform(glm::vec3(0), glm::vec3(0), glm::vec3(0));
    for (int i = 0; i < 10; i++)
    {
        transform = Transform(glm::vec3(-30 + 6.0f * i, -9, -30), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
        RenderObject object = RenderObject(duckModel, materials[i % 5], transform);
        if (i % 5 == 4)
        {
            lights->AddObject(object);
        }
        accelerationStructure->AddObject(object);
        std::cout << "Object Added" << std::endl;
    }
    transform.scale = glm::vec3(40, 40, 40);
    RenderObject ceilingObject = RenderObject(planeModel, ceiling, Transform(glm::vec3(0, 10, -40), glm::vec3(0, 0, 0), glm::vec3(40, 40, 40)));
    RenderObject bigDuck = RenderObject(duckModel, red_rubber, Transform(glm::vec3(0, -20, -40), glm::vec3(0, 0, 0), glm::vec3(40, 40, 40)));
    RenderObject floorObject = RenderObject(planeModel, ivory, Transform(glm::vec3(0, -20, -40), glm::vec3(0, 0, 0), glm::vec3(40, 40, 40)));

    lights->AddObject(ceilingObject);
    accelerationStructure->AddObject(ceilingObject);
    accelerationStructure->AddObject(bigDuck);
    accelerationStructure->AddObject(floorObject);
}

// high poly ducks next to each other
void GenerateScene3(AccelStruct& accelerationStructure, AccelStruct& lights)
{
    Material      ivory(Material::Type::Dielectric, glm::vec3(0.4, 0.4, 0.3), glm::vec3(0.4, 0.4, 0.3), 1.0, 50.);
    Material      glass(Material::Type::Dielectric, glm::vec3(0.6, 0.7, 0.8), glm::vec3(0.6, 0.7, 0.8), 1.5, 125.);
    Material red_rubber(Material::Type::Dielectric, glm::vec3(0.3, 0.1, 0.1), glm::vec3(0.3, 0.1, 0.1), 1.0, 10.);
    Material     mirror(Material::Type::Mirror, glm::vec3(0.8), glm::vec3(1.0, 1.0, 1.0), 1.0, 1425.);
    Material   emissiveDuck(Material::Type::Emmisive, glm::vec3(1.0, 1.0, 1.0), glm::vec3(1.0, 1.0, 1.0), 1.0, 1425.);
    Material   ceiling(Material::Type::Emmisive, glm::vec3(1.0, 1.0, 1.0), glm::vec3(1.0, 1.0, 1.0), 1.0, 1425.);
    emissiveDuck.emittedRadiance = glm::vec3(1.0f, 1.0f, 1.0f);
    ceiling.emittedRadiance = glm::vec3(1.0f, 1.0f, 1.0f);

    std::vector<Material> materials;
    materials.reserve(4);
    materials.push_back(ivory);
    materials.push_back(glass);
    materials.push_back(red_rubber);
    materials.push_back(mirror);
    materials.push_back(emissiveDuck);

    std::vector<RenderObject> renderObjects;
    Model duckModel = Model("../duck.obj");
    Model duck1Model = Model("../duck1.obj");
    Model duck2Model = Model("../duck2.obj");
    Model duckSubdividedModel = Model("../duck_subdivided.obj");
    Model sphereModel = Model("../sphere.obj");
    Model planeModel = Model("../plane.obj");
    Transform transform = Transform(glm::vec3(0), glm::vec3(0), glm::vec3(0));
    for (int i = 0; i < 10; i++)
    {
        transform = Transform(glm::vec3(-20 + 5.0f * i, -9, -15), glm::vec3(0, 0, 0), glm::vec3(0.5f, 0.5f, 0.5f));
        RenderObject object = RenderObject(duckSubdividedModel, materials[i % 5], transform);
        if (i % 5 == 4)
        {
            lights->AddObject(object);
        }
        accelerationStructure->AddObject(object);
        std::cout << "Object Added" << std::endl;
    }
    transform.scale = glm::vec3(40, 40, 40);
    RenderObject ceilingObject = RenderObject(planeModel, ceiling, Transform(glm::vec3(0, 10, -40), glm::vec3(0, 0, 0), glm::vec3(40, 40, 40)));
    RenderObject bigDuck = RenderObject(duckSubdividedModel, red_rubber, Transform(glm::vec3(0, -20, -40), glm::vec3(0, 0, 0), glm::vec3(40, 40, 40)));
    RenderObject floorObject = RenderObject(planeModel, ivory, Transform(glm::vec3(0, -20, -40), glm::vec3(0, 0, 0), glm::vec3(40, 40, 40)));

    lights->AddObject(ceilingObject);
    accelerationStructure->AddObject(ceilingObject);
    accelerationStructure->AddObject(bigDuck);
    accelerationStructure->AddObject(floorObject);
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

    AccelStruct accelerationStructure = std::make_unique<BVH>();
    AccelStruct lights = std::make_unique<BVH>();

    //GenerateScene1(accelerationStructure, lights);
    GenerateScene2(accelerationStructure, lights);
    //GenerateScene3(accelerationStructure, lights);

    accelerationStructure->Build(true);
    lights->Build(true);
    std::cout << "AccelerationStructure Built" << std::endl;

    render(accelerationStructure, lights);

    return 0;
}
