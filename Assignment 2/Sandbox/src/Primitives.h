#pragma once

#include "Include.h"
#include "Ray.h"

class Primitive
{
public:
	~Primitive() = default;

	virtual bool Intersect(Ray& ray) const = 0;
	virtual float Area() const = 0;
};


struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;

	Vertex() = default;

	Vertex(const glm::vec3& position, const glm::vec2& texCoord) :
		position{ position }, normal{ glm::vec3(0) }, texCoord{texCoord}
	{}

	Vertex(const glm::vec3 & position, const glm::vec3 & normal = glm::vec3(0), const glm::vec2 & texCoord = glm::vec2(0)) :
		position{ position }, normal{ glm::normalize(normal) }, texCoord{ texCoord }
	{}
};


class Triangle : public Primitive
{
private:
	Vertex m_Vertex0;
	Vertex m_Vertex1;
	Vertex m_Vertex2;

	glm::vec3 m_Normal;
public:
	Triangle(const Vertex& vertex0, const Vertex& vertex1, const Vertex& vertex2):
		m_Vertex0{ vertex0 }, m_Vertex1{ vertex1 }, m_Vertex2{ vertex2 }
	{
		m_Normal = glm::normalize(glm::cross(vertex2.position - vertex0.position, vertex1.position - vertex0.position));
	}

	virtual bool Intersect(Ray& ray) const override;
	virtual float Area() const override;

	Vertex GetVertex0() const	{ return m_Vertex0; }
	Vertex GetVertex1() const	{ return m_Vertex1; }
	Vertex GetVertex2() const	{ return m_Vertex2; }
	glm::vec3 GetNormal() const { return m_Normal; }
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

	virtual bool Intersect(Ray& ray) const override;
	virtual float Area() const override;
};