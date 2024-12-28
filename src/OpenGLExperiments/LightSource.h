#pragma once

#include "Structs.h"

#include <stdlib.h>
#include <cmath>

namespace Raytracer2Dns
{
	class LightSource
	{
	public:
		Vec2 position;

		LightSource(Vec2 position);

		void getRay(Ray& ray);
	};
}
