#define USEBVH

#include "Scene.h"
#include <limits>

using namespace Raytracer2Dns;

GLfloat random(GLfloat low, GLfloat high)
{
	return low + static_cast <GLfloat> (rand()) / (static_cast <GLfloat> (RAND_MAX / (high - low)));
}

Scene::Scene(GLbyte maxPathLength)
{
	tracedLines = std::vector<Line>(maxPathLength);
	obstacles = std::vector<Obstacle*>();
	//obstacles.push_back(new LineObstacle({ -100.0, -50.0 }, { -110.0, -40.0 }));
	//obstacles.push_back(new LineObstacle({ -200.0, -100.0 }, { -210.0, -110.0 }));
	//obstacles.push_back(new LineObstacle({ 100.0, -300.0 }, { 110.0, -310.0 }));
	//obstacles.push_back(new LineObstacle({ 200.0, -250.0 }, { 210.0, -200.0 }));

	//obstacles.push_back(new LineObstacle({ 250.0 - 10, 0.0 }, { 350.0 + 10, 100.0 }));
	//obstacles.push_back(new LineObstacle({ 250.0 - 20, 0.0 }, { 350.0 + 20, 100.0 }));
	//obstacles.push_back(new LineObstacle({ 250.0 - 30, 0.0 }, { 350.0 + 30, 100.0 }));
	//obstacles.push_back(new LineObstacle({ 250.0 - 40, 0.0 }, { 350.0 + 40, 100.0 }));
	//obstacles.push_back(new LineObstacle({ 250.0 - 50, 0.0 }, { 350.0 + 50, 100.0 }));
	//obstacles.push_back(new LineObstacle({ 250.0 - 60, 0.0 }, { 350.0 + 60, 100.0 }));
	//obstacles.push_back(new LineObstacle({ 250.0 - 70, 0.0 }, { 350.0 + 70, 100.0 }));
	//obstacles.push_back(new LineObstacle({ 250.0 - 80, 0.0 }, { 350.0 + 80, 100.0 }));
	//obstacles.push_back(new LineObstacle({ 250.0 - 90, -200.0 }, { 350.0 + 90, -200.0 }));

	// bvh testing
	//int a = 100;
	//for (int x = 0; x < 10; x++) {
	//	for (int y = 0; y < 5; y++) {
	//		obstacles.push_back(new LineObstacle({ -620.0f + x * a, -380.0f + y * a }, { -610.0f + x * a, -370.0f + y * a }));
	//	}
	//}
	//
	obstacles.push_back(new LineObstacle({ -620.0, -390.0 }, { -630.0, 390.0 }));
	obstacles.push_back(new LineObstacle({ 630.0, -390.0 }, { 620.0, 390.0 }));
	Vec2 t = { -10, 10 };
	for (int c = 2; c < 100000; c++)
	{
		Vec2 p = { random(-620.0, 620.0), random(-380.0, 380.0) };
		obstacles.push_back(new LineObstacle(p + t, p - t));
	}
	lightSource = new LightSource({ -100.0, 1.0 });

	obstacleCount = obstacles.size();

	for (GLuint i = 0; i < obstacleCount; i++)
	{
		obstacles[i]->getDrawPoints(obstaclesDrawPoints);
	}

	bvh = BVH2D(obstacles);
	bvhDrawPoints = std::vector<Vec2>(bvh.nodesCount * 8);
	bvh.getDrawPoints(bvhDrawPoints);
}

Scene::~Scene()
{
	delete lightSource;
	for (GLint i = 0; i < obstacleCount; i++)
	{
		delete obstacles[i];
	}
}

GLint Scene::trace(Ray& ray, Line* result, GLbyte maxPathLength)
{
	GLint linesFound = 0;

	GLint obstIndex = -1;
	for (GLint p = 0; p < maxPathLength; p++)
	{
		linesFound++;
		result[p].start = ray.origin;
//GLfloat hitDist = fMax;
#ifndef USEBVH
		GLfloat hitDist = fMax;
		for (GLint i = 0; i < obstacleCount; i++)
		{
			if (i != obstIndex)
			{
				obstacles[i]->intersect(ray, i);
				if (hitDist > ray.hit.intersectionDistance)
				{
					obstIndex = i;
					hitDist = ray.hit.intersectionDistance;
				}
			}
		}
#else
		bvh.IntersectBVH(ray);
		GLfloat hitDist = ray.hit.intersectionDistance;
		obstIndex = ray.hit.obstIndex;
#endif

		if (obstIndex == -1)
		{
			result[p].end.x = ray.direction.x * 10000.0 + ray.origin.x; // out of screen, most likely
			result[p].end.y = ray.direction.y * 10000.0 + ray.origin.y;
			break;
		}

		result[p].end.x = ray.direction.x * hitDist + ray.origin.x;
		result[p].end.y = ray.direction.y * hitDist + ray.origin.y;

		Vec2 newRayDir = obstacles[obstIndex]->interact(ray, result[p].end, ray.hit.normal);
		if (newRayDir.x == 0 && newRayDir.y == 0)
		{
			break;
		}
		ray.origin = result[p].end;
		ray.direction = newRayDir;
		ray.ResetHit();
	}

	return linesFound;
}

void Scene::addObstacle(Obstacle* obst)
{
	obstacles.push_back(obst);

	obstacleCount = obstacles.size();

	for (GLuint i = 0; i < obstacleCount; i++)
	{
		obstacles[i]->getDrawPoints(obstaclesDrawPoints);
	}

	bvh = BVH2D(obstacles);
	bvhDrawPoints = std::vector<Vec2>(bvh.nodesCount * 8);
	bvh.getDrawPoints(bvhDrawPoints);
}
