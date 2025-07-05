#pragma once

#include "Hazel/Core.h"

namespace Hazel {

	class HAZEL_API GraphicsContext
	{
	public:
		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;
	};

}