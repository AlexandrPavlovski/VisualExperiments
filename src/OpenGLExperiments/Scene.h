#pragma once

#include "LightSource.h"
#include "Obstacles.h"
#include "BVH2D.h"

#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace Raytracer2Dns
{
	class Scene
	{
	public:
		LightSource* lightSource;
		std::vector<Obstacle*> obstacles;

		std::vector<Vec2> obstaclesDrawPoints;
		GLuint obstaclesDrawPointsCount = 0;

		std::vector<Vec2> bvhDrawPoints;

		Scene() = default;
		Scene(GLbyte maxPathLength);
		~Scene();
		GLint trace(Ray& ray, Line *result, GLbyte maxPathLength);
		void addObstacle(Obstacle* obst);
	private:
		GLbyte maxPathLength = 0;
		std::vector<Line> tracedLines;

		GLuint obstacleCount = 0;

		BVH2D bvh;
	};
}
