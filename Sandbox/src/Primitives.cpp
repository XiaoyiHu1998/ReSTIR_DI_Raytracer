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