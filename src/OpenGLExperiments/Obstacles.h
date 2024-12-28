#pragma once

#include "Structs.h"

#include <vector>

namespace Raytracer2Dns
{
	struct Obstacle
	{
	public:
		Vec2 centroid;
		Vec2 aabbMin, aabbMax;
		GLuint drawPointsCount;

		virtual void intersect(Ray& ray, GLuint obstIndex) = 0;
		virtual Vec2 interact(Ray& ray, Vec2 contactPoint, Vec2 normal) = 0;

		virtual void getDrawPoints(std::vector<Vec2>& points) = 0;
	};

	struct LineObstacle : public Obstacle
	{
	public:
		Vec2 start, end;

		LineObstacle(Vec2 start, Vec2 end);

		void intersect(Ray& ray, GLuint obstIndex) override;
		Vec2 interact(Ray& ray, Vec2 contactPoint, Vec2 normal) override;

		void getDrawPoints(std::vector<Vec2>& points) override;
	};
}
