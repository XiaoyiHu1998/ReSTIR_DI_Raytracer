#ifndef OBJECT_CPP
#define OBJECT_CPP

#include "model.h"

struct Transform {
	Vec3f translation, rotation, scale;

	Transform(Vec3f translation, Vec3f rotation, Vec3f scale) :
		translation{ translation }, rotation{ rotation }, scale{ scale }
	{}

	void GetTransformMatrix()
	{
		return;
	}
};

class RenderObject {
	Model m_Model;
	Transform m_transform;
};


#endif //OBJECT_CPP