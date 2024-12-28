#pragma once

#include "Structs.h"
#include "Obstacles.h"

#include <vector>

#define BINS 8

namespace Raytracer2Dns
{
	class BVH2D
	{
	public:
		GLuint nodesCount = 0;

		BVH2D() = default;
		BVH2D(const std::vector<Obstacle*>& obstacles);

		void Build();
		void IntersectBVH(Ray& ray);
		void getDrawPoints(std::vector<Vec2>& points);

	private:
		BVHNode* bvhNodes = 0;
		GLuint rootNodeIndex = 0, nodesUsed = 1;

		std::vector<Obstacle*> obstacles;
		GLuint* obstaclesIndexes = 0;

		void UpdateNodeBounds(GLuint nodeIndex);
		void Subdivide(GLuint nodeIndex);
		GLfloat IntersectAABB(const Ray& ray, const Vec2 min, const Vec2 max);
		GLfloat EvaluateSAH(BVHNode& node, GLubyte axis, GLfloat pos);
		GLfloat FindBestSplitPLane(BVHNode& node, GLint& axis, GLfloat& splitPos);
	};
}

