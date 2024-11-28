#include "BVH.h"

BVH::BVH()
{
	triangles = {};
	bvhNodes = {};

	triangles.reserve(10000 * 3);
	bvhNodes.reserve(30);
}

bool BVH::RayHit(glm::vec3 origin, glm::vec3 direction, float& tnear)
{
	return false;
}

void BVH::AddObject(const RenderObject& object)
{
	const Model& model = object.m_Model;
	int faceCount = model.nfaces();

	triangles.reserve(triangles.size() + faceCount * 3);

	for (int faceIndex = 0; faceIndex < faceCount; faceIndex++) {
		triangles.emplace_back(Triangle(model.point(model.vert(faceIndex, 0)), model.point(model.vert(faceIndex, 1)), model.point(model.vert(faceIndex, 2))));
	}
}

void BVH::Build(bool useHeuristic)
{

}
