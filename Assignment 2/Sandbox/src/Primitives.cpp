#include "Primitives.h"
#include "Utils.h"

bool Triangle::Intersect(Ray& ray) const 
{
	const glm::vec3 edge1 = m_Vertex1.position - m_Vertex0.position;
	const glm::vec3 edge2 = m_Vertex2.position - m_Vertex0.position;
	const glm::vec3 h = glm::cross(ray.direction, edge2);
	const float a = glm::dot(edge1, h);

	if (a > -0.0001f && a < 0.0001f) return false; // ray parallel to triangle

	const float f = 1 / a;
	const glm::vec3 s = ray.origin - m_Vertex0.position;
	const float u = f * glm::dot(s, h);

	if (u < 0 || u > 1) return false;

	const glm::vec3 q = glm::cross(s, edge1);
	const float v = f * glm::dot(ray.direction, q);

	if (v < 0 || u + v > 1) return false;

	const float t = f * glm::dot(edge2, q);

	if (t > 0.0001f)
	{
		ray.hitInfo.distance = ray.hitInfo.distance < t ? ray.hitInfo.distance : t;
		return true;
	}

	return false;
}

float Triangle::Area() const
{
	//Cos method
	glm::vec3 edge1 = m_Vertex1.position - m_Vertex0.position;
	glm::vec3 edge2 = m_Vertex2.position - m_Vertex1.position;
	float lengthEdge1 = glm::length(edge1);
	float lengthEdge2 = glm::length(edge2);
	
	float dotProduct = glm::dot(edge1, edge2);
	float cosTheta = dotProduct / (lengthEdge1 * lengthEdge2);
	float sinTheta = glm::sqrt(1 - cosTheta * cosTheta);
	
	return 0.5f * lengthEdge1 * lengthEdge2 * sinTheta;
}

bool Sphere::Intersect(Ray& ray) const
{
	glm::vec3 rayOriginSphereVector = ray.origin - position;
	float a = glm::dot(ray.direction, ray.direction);
	float b = 2.0f * glm::dot(rayOriginSphereVector, ray.direction);
	float c = glm::dot(rayOriginSphereVector, rayOriginSphereVector) - radius * radius;
	float D = b * b - 4 * a * c;
	
	if (D < 0)
		return false;

	float denominator = 0.5f / a;
	float p = sqrt(D) * denominator;
	float q = -b * denominator;

	float t0 = p + q;
	float t1 = p - q;

	if (t0 < 0 && t1 < 0) return false;
	else if (t0 < 0 || t1 < t0) std::swap(t0, t1);

	ray.hitInfo.distance = t0;

	return true;
}

float Sphere::Area() const
{
	return 4 * glm::pi<float>() * radius;
}

const glm::vec3 Triangle::GetRandomPoint(uint32_t& seed)
{
	// Coordinates on parallelogram
	float u = Utils::RandomFloat(seed);
	float v = Utils::RandomFloat(seed);

	// Flip coordinates on parallelogram outisde of triangle
	if (u + v > 1)
	{
		u = 1 - u;
		v = 1 - v;
	}

	glm::vec3 edge1 = m_Vertex1.position - m_Vertex0.position;
	glm::vec3 edge2 = m_Vertex2.position - m_Vertex0.position;

	return m_Vertex0.position + u * edge1 + v * edge2;
}
