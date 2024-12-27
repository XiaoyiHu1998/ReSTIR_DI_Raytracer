#include "Mesh.h"

bool GeometryLoader::LoadGeometryFromFile(const std::string& filepath, std::vector<Triangle>& triangles)
{
	triangles.clear();
	std::vector<Vertex> vertices;
	tinyobj::ObjReader objReader;

	objReader.ParseFromFile(filepath);

	if (!objReader.Valid())
	{
		std::cout << "Warning Loading OBJ file: " << objReader.Warning() << std::endl;
		std::cout << "Error Loading OBJ file: " << objReader.Error() << std::endl;
		return false;
	}

	tinyobj::attrib_t attributes = objReader.GetAttrib();
	for (int shapeIndex = 0; shapeIndex < objReader.GetShapes().size(); shapeIndex++)
	{
		const tinyobj::shape_t& shape = objReader.GetShapes()[shapeIndex];
		const tinyobj::mesh_t& mesh = shape.mesh;

		for (int meshIndex = 0; meshIndex < mesh.indices.size(); meshIndex++)
		{
			tinyobj::index_t tinyObjIndex = mesh.indices[meshIndex];

			glm::vec3 position =
			{
				attributes.vertices[tinyObjIndex.vertex_index * 3 + 0],
				attributes.vertices[tinyObjIndex.vertex_index * 3 + 1],
				attributes.vertices[tinyObjIndex.vertex_index * 3 + 2]
			};

			glm::vec3 normal =
			{
				attributes.normals[tinyObjIndex.normal_index * 3 + 0],
				attributes.normals[tinyObjIndex.normal_index * 3 + 1],
				attributes.normals[tinyObjIndex.normal_index * 3 + 2]
			};

			glm::vec2 texCoord =
			{
				attributes.texcoords[tinyObjIndex.texcoord_index * 2+ 0],
				attributes.texcoords[tinyObjIndex.texcoord_index * 2+ 1]
			};

			vertices.emplace_back(position, normal, texCoord);
		}
	}
	std::cout << "vertexCount: " << vertices.size() << std::endl;
	std::cout << "trianglesCount: " << vertices.size() / 3 << std::endl;

	triangles.reserve(vertices.size() / 3);
	for (int i = 0; i < vertices.size(); i += 3)
	{
		std::cout << "Triangle " << i / 3 << ": " << glm::to_string(vertices[i + 0].position) << " | " << glm::to_string(vertices[i + 1].position) << " | " << glm::to_string(vertices[i + 2].position) << std::endl;
		triangles.emplace_back(vertices[i + 0], vertices[i + 1], vertices[i + 2]);
	}

	return true;
}
