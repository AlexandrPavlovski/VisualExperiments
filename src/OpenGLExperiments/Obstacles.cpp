#include "Obstacles.h"
#include <limits>

using namespace Raytracer2Dns;

LineObstacle::LineObstacle(Vec2 start, Vec2 end)
{
	this->start = start;
	this->end = end;
	centroid = (start + end) / 2.0;
	aabbMin = { fminf(start.x, end.x), fminf(start.y, end.y) };
	aabbMax = { fmaxf(start.x, end.x), fmaxf(start.y, end.y) };
	drawPointsCount = 2;
}
void LineObstacle::intersect(Ray& ray, GLuint obstIndex)
{
return;
	Vec2 sT = end - start;
	Vec2 sN = { -sT.y, sT.x };
	GLfloat t = dot(sN, start - ray.origin) / dot(sN, ray.direction);
	GLfloat u = dot(sT, ray.origin + ray.direction * t - start);
	//if (t < 1e-10 || t > 1e+10 || u < 0.0 || u > dot(sT, sT))
	//{
	//	ray.hit.intersectionDistance = fMax;
	//}
	//else
	if (t > 1e-10 && t < 1e+10 && u > 0.0 && u < dot(sT, sT))
	{
		ray.hit.intersectionDistance = t;
		ray.hit.normal = normalize(sN);
		ray.hit.obstIndex = obstIndex;
	}
}
Vec2 LineObstacle::interact(Ray& ray, Vec2 contactPoint, Vec2 normal)
{
	//return { ray.direction.x, ray.direction.y };
	//return { 1, 0 };
	//return { 0, 1 };
	//ray.direction = ray.direction * -1;
	auto t = dot(ray.direction, normal);
	auto t2 = normal * t;
	auto t3 = t2 * 2;
	auto t4 = ray.direction - t3;
	return ray.direction - normal * dot(ray.direction, normal) * 2;
}
void LineObstacle::getDrawPoints(std::vector<Vec2>& points)
{
	points.push_back(start);
	points.push_back(end);
}
