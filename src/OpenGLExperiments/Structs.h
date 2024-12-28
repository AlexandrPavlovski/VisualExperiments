#pragma once

#include <cmath>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

static const float fMax = 1e30f;
static const float fMin = -1e30f;

namespace Raytracer2Dns
{
	struct Vec2
	{
		GLfloat x = 0;
		GLfloat y = 0;

		Vec2()
		{
		}

		Vec2(GLfloat value)
		{
			x = value;
			y = value;
		}

		Vec2(GLfloat x, GLfloat y)
		{
			this->x = x;
			this->y = y;
		}

		Vec2 inline operator+(Vec2 a) {
			return { a.x + x, a.y + y };
		}
		Vec2 inline operator-(Vec2 a) {
			return { x - a.x, y - a.y };
		}
		Vec2 inline operator*(Vec2 a) {
			return { a.x * x, a.y * y };
		}
		Vec2 inline operator*(GLfloat a) {
			return { a * x, a * y };
		}
		Vec2 inline operator/(GLfloat a) {
			return { x / a, y / a };
		}
		GLfloat inline operator[](GLubyte axis)
		{
			return axis == 0 ? x : y;
		}

		void shrink(GLfloat val)
		{
			x = fminf(x, val);
			y = fminf(y, val);
		}
		void shrink(Vec2 val)
		{
			x = fminf(x, val.x);
			y = fminf(y, val.y);
		}
		void grow(GLfloat val)
		{
			x = fmaxf(x, val);
			y = fmaxf(y, val);
		}
		void grow(Vec2 val)
		{
			x = fmaxf(x, val.x);
			y = fmaxf(y, val.y);
		}
	};

	struct Intersection
	{
		GLfloat intersectionDistance;
		Vec2 normal;
		GLint obstIndex;
	};

	struct Ray
	{
		Vec2 origin;
		Vec2 direction;
		Vec2 rDirection; // reciprocal
		Intersection hit;

		Ray()
		{
			hit.intersectionDistance = fMax;
			hit.obstIndex = -1;
		}

		Ray(Vec2 origin, Vec2 direction)
		{
			this->origin = origin;
			this->direction = direction;
			rDirection = { 1.0f / direction.x, 1.0f / direction.y };
		}

		void ResetHit()
		{
			hit.intersectionDistance = fMax;
			hit.normal = { 0, 0 };
			hit.obstIndex = -1;
		}
	};

	struct Line
	{
		Vec2 start, end;
	};

	struct BVHNode
	{
		Vec2 aabbMin, aabbMax;
		GLuint leftOrFirst; // if leaf, stores index of first obstacle, else index of left node (right node index is always +1)
		GLuint obstCount;

		Vec2 memAlign; // to make struct 32 byte

		bool isLeaf()
		{
			return obstCount > 0;
		}

		GLfloat traceCost()
		{
			Vec2 e = aabbMax - aabbMin;
			GLfloat halfPerimeter = e.x + e.y;
			return halfPerimeter * obstCount;
		}
	};

	struct aabb2d
	{
		Vec2 min = fMax;
		Vec2 max = fMin;

		void grow(Vec2 p)
		{
			min.shrink(p);
			max.grow(p);
		}
		void grow(aabb2d aabb)
		{
			grow(aabb.min);
			grow(aabb.max);
		}

		GLfloat halfPerimeter()
		{
			Vec2 e = max - min;
			return e.x + e.y;
		}
	};

	struct Bin
	{
		aabb2d bounds;
		GLint obstCount = 0;
	};

	inline GLfloat dot(Vec2 a, Vec2 b)
	{
		return a.x * b.x + a.y * b.y;
	}
	inline GLfloat length(Vec2 a)
	{
		return std::sqrt(a.x * a.x + a.y * a.y);
	}
	inline Vec2 normalize(Vec2 a)
	{
		GLfloat l = length(a);
		return { a.x / l, a.y / l };
	}
}
