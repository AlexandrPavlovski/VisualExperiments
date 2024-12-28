#include "LightSource.h"

#define PI 3.14159265358979323846

using namespace Raytracer2Dns;

LightSource::LightSource(Vec2 position)
{
	this->position = position;
}

void LightSource::getRay(Ray& ray)
{
	ray.origin = position;

	GLfloat r = rand() / (GLfloat)(RAND_MAX + 1); // r is in range [0.0, 1.0)
	GLfloat theta = r * 2 * PI;
//theta = theta / 100 - 0.6;
	ray.direction.x = cos(theta);
	ray.direction.y = sin(theta);

	ray.rDirection.x = 1.0f / ray.direction.x;
	ray.rDirection.y = 1.0f / ray.direction.y;

	ray.ResetHit();
}
