#pragma once

#include "Ray.h"

class Primitive
{
public:
	~Primitive() = default;

	virtual bool Intersect(Ray& ray) = 0;
	virtual float Area() = 0;
};


class Triangle : public Primitive
{
private:
	glm::vec3 m_Vertex0;
	glm::vec3 m_Vertex1;
	glm::vec3 m_Vertex2;

	glm::vec3 m_Normal;
public:
	Triangle(const glm::vec3& vertex0, const glm::vec3& vertex1, const glm::vec3& vertex2):
		m_Vertex0{ vertex0 }, m_Vertex1{ vertex1 }, m_Vertex2{ vertex2 }
	{
		m_Normal = glm::normalize(glm::cross(vertex2 - vertex0, vertex1 - vertex0));
	}

	virtual bool Intersect(Ray& ray);
	virtual float Area();
};


class Sphere : public Primitive
{
private:
	glm::vec3 m_Position;
	float m_Radius;

public:
	Sphere(const glm::vec3& position, float radius):
		m_Position{ position }, m_Radius{ radius }
	{}

	virtual bool Intersect(Ray& ray);
	virtual float Area();
};