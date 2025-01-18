# pragma once

#include "tiny_obj_loader.h"

#include "Include.h"
#include "Primitives.h"
#include "Transform.h"
#include "Material.h"

namespace GeometryLoader
{
	bool LoadGeometryFromFile(const std::string& filepath, std::vector<Triangle>& triangles);
}

class Mesh
{
private:
	std::vector<Triangle> m_Triangles;
	Transform m_Transform;
	Material m_Material;
public:
	Mesh(const std::vector<Triangle>& triangles, const Transform& transform) :
		m_Triangles{ triangles }, m_Transform{ transform }, m_Material{ Material(Material::Type::Emissive, 0, glm::vec3(0), glm::vec3(0.8f, 0.2f, 0.2f), 1.0f) }
	{}

	Mesh(const std::vector<Triangle>&triangles, const Transform & transform, const Material& material) :
		m_Triangles{ triangles }, m_Transform{ transform }, m_Material{ material }
	{}

	~Mesh() = default;

	std::vector<Triangle> GetTriangles() const { return m_Triangles; }
	Transform GetTransform() const { return m_Transform; }
	Material GetMaterial() const { return m_Material; }

	void SetTransform(const Transform& transform) { m_Transform = transform; }
	void SetMaterial(const Material& material) { m_Material = material; }
};