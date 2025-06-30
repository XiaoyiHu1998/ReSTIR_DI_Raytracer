# pragma once

#include "tiny_obj_loader.h"

#include "Include.h"
#include "Primitives.h"
#include "Transform.h"

namespace GeometryLoader
{
	bool LoadGeometryFromFile(const std::string& filepath, std::vector<Triangle>& triangles);
}

class Mesh
{
private:
	std::vector<Triangle> m_Triangles;
	Transform m_Transform;
public:
	Mesh(const std::vector<Triangle>& triangles, const Transform& transform) :
		m_Triangles{ triangles }, m_Transform{ transform }
	{}

	~Mesh() = default;

	std::vector<Triangle> GetTriangles() const { return m_Triangles; }
	Transform GetTransform() const { return m_Transform; }

	void SetTransform(const Transform& transform) { m_Transform = transform; }
};