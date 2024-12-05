#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

struct Triangle
{
	glm::vec3 vertex0, vertex1, vertex2;
	glm::vec3 centroid;

	Triangle(glm::vec3 vertex0, glm::vec3 vertex1, glm::vec3 vertex2) :
		vertex0{ vertex0 }, vertex1{ vertex1 }, vertex2{ vertex2 }
	{
		centroid = (vertex0 + vertex1 + vertex2) * 0.3333f;
	}

	bool Intersect(glm::vec3 origin, glm::vec3 direction, float& tnear)
	{
		const glm::vec3 edge1 = vertex1 - vertex0;
		const glm::vec3 edge2 = vertex2 - vertex0;
		const glm::vec3 h = glm::cross(direction, edge2);
		const float a = glm::dot(edge1, h);
		if (a > -0.0001f && a < 0.0001f) return false; // ray parallel to triangle
		const float f = 1 / a;
		const glm::vec3 s = origin - vertex0;
		const float u = f * glm::dot(s, h);
		if (u < 0 || u > 1) return false;
		const glm::vec3 q = glm::cross(s, edge1);
		const float v = f * glm::dot(direction, q);
		if (v < 0 || u + v > 1) return false;
		const float t = f * glm::dot(edge2, q);
		if (t > 0.0001f) {
			tnear = tnear < t ? tnear : t;
			return true;
		}

		return false;
	}
};

#endif //TRIANGLE_HPP