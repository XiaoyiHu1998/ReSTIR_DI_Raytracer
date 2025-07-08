#include "GeometryLoader.h"

bool GeometryLoader::LoadObj(const std::string& filepath, std::vector<tinybvh::bvhvec4>& vertices)
{
	vertices.clear();
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

			float x = attributes.vertices[tinyObjIndex.vertex_index * 3 + 0];
			float y = attributes.vertices[tinyObjIndex.vertex_index * 3 + 1];
			float z = attributes.vertices[tinyObjIndex.vertex_index * 3 + 2];

			vertices.emplace_back(x, y, z, 0.0f);
		}
	}

	return true;
}