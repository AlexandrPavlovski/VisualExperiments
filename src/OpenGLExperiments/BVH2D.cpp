#include "BVH2D.h"

using namespace Raytracer2Dns;

BVH2D::BVH2D(const std::vector<Obstacle*>& obstacles)
{
	GLint N = obstacles.size();
	this->obstacles = obstacles;
	
	obstaclesIndexes = new GLuint[N];
	for (GLuint i = 0; i < N; i++)
	{
		obstaclesIndexes[i] = i;
	}

	bvhNodes = (BVHNode*)_aligned_malloc(sizeof(BVHNode) * N * 2, 64);

	BVHNode& root = bvhNodes[rootNodeIndex];
	root.leftOrFirst = 0;
	root.obstCount = N;
	nodesCount = 1;

	Build();
}

void BVH2D::Build()
{
	UpdateNodeBounds(rootNodeIndex);
	Subdivide(rootNodeIndex);
}

void BVH2D::IntersectBVH(Ray& ray)
{
	BVHNode* node = &bvhNodes[rootNodeIndex], *stack[64];
	GLuint stackPtr = 0;
	while (true)
	{
		if (node->isLeaf())
		{
			for (GLuint i = 0; i < node->obstCount; i++)
			{
				GLuint idx = obstaclesIndexes[node->leftOrFirst + i];
				obstacles[idx]->intersect(ray, idx);
			}

			if (stackPtr == 0)
				break;
			else
				node = stack[--stackPtr];

			continue;
		}

		BVHNode* child1 = &bvhNodes[node->leftOrFirst];
		BVHNode* child2 = &bvhNodes[node->leftOrFirst + 1];
		GLfloat dist1 = IntersectAABB(ray, child1->aabbMin, child1->aabbMax);
		GLfloat dist2 = IntersectAABB(ray, child2->aabbMin, child2->aabbMax);

		if (dist1 > dist2)
		{
			std::swap(dist1, dist2);
			std::swap(child1, child2);
		}

		if (dist1 == fMax)
		{
			if (stackPtr == 0)
				break;
			else
				node = stack[--stackPtr];
		}
		else
		{
			node = child1;
			if (dist2 != fMax)
			{
				stack[stackPtr++] = child2;
			}
		}
	}
}

void BVH2D::getDrawPoints(std::vector<Vec2>& points)
{
	for (GLuint i = 0; i < nodesCount; i++)
	{
		BVHNode& node = bvhNodes[i];
		points[i * 8 + 0] = node.aabbMin;
		points[i * 8 + 1] = { node.aabbMin.x, node.aabbMax.y };
		points[i * 8 + 2] = { node.aabbMin.x, node.aabbMax.y };
		points[i * 8 + 3] = node.aabbMax;
		points[i * 8 + 4] = node.aabbMax;
		points[i * 8 + 5] = { node.aabbMax.x, node.aabbMin.y };
		points[i * 8 + 6] = { node.aabbMax.x, node.aabbMin.y };
		points[i * 8 + 7] = node.aabbMin;
	}
}


void BVH2D::UpdateNodeBounds(GLuint nodeIndex)
{
	BVHNode& node = bvhNodes[nodeIndex];
	node.aabbMin = Vec2(1e30f);
	node.aabbMax = Vec2(-1e30f);
	for (GLuint first = node.leftOrFirst, i = 0; i < node.obstCount; i++)
	{
		GLuint leafIndex = obstaclesIndexes[first + i];
		Obstacle* leaf = obstacles[leafIndex];
		node.aabbMin = { fminf(node.aabbMin.x, leaf->aabbMin.x), fminf(node.aabbMin.y, leaf->aabbMin.y) };
		node.aabbMax = { fmaxf(node.aabbMax.x, leaf->aabbMax.x), fmaxf(node.aabbMax.y, leaf->aabbMax.y) };
	}
}

void BVH2D::Subdivide(GLuint nodeIndex)
{
	BVHNode& node = bvhNodes[nodeIndex];
	GLint axis;
	GLfloat splitPos;
	GLfloat noSplitCost = node.traceCost();
	GLfloat splitCost = FindBestSplitPLane(node, axis, splitPos);
	if (splitCost > noSplitCost)
	{
		return;
	}

	nodesCount += 2;
	
	GLint i = node.leftOrFirst;
	GLint j = i + node.obstCount - 1;
	while (i <= j)
	{
		if (obstacles[obstaclesIndexes[i]]->centroid[axis] < splitPos)
		{
			i++;
		}
		else
		{
			std::swap(obstaclesIndexes[i], obstaclesIndexes[j--]);
		}
	}

	GLint leftCount = i - node.leftOrFirst;
	if (leftCount == 0 || leftCount == node.obstCount)
	{
		return;
	}

	GLint leftChildIndex = nodesUsed++;
	GLint rightChildIndex = nodesUsed++;
	bvhNodes[leftChildIndex].leftOrFirst = node.leftOrFirst;
	bvhNodes[leftChildIndex].obstCount = leftCount;
	bvhNodes[rightChildIndex].leftOrFirst = i;
	bvhNodes[rightChildIndex].obstCount = node.obstCount - leftCount;
	node.leftOrFirst = leftChildIndex;
	node.obstCount = 0;

	UpdateNodeBounds(leftChildIndex);
	UpdateNodeBounds(rightChildIndex);

	Subdivide(leftChildIndex);
	Subdivide(rightChildIndex);
}

GLfloat BVH2D::IntersectAABB(const Ray& ray, const Vec2 min, const Vec2 max)
{
	GLfloat tx1 = (min.x - ray.origin.x) * ray.rDirection.x;
	GLfloat tx2 = (max.x - ray.origin.x) * ray.rDirection.x;
	GLfloat tmin = fminf(tx1, tx2);
	GLfloat tmax = fmaxf(tx1, tx2);

	GLfloat ty1 = (min.y - ray.origin.y) * ray.rDirection.y;
	GLfloat ty2 = (max.y - ray.origin.y) * ray.rDirection.y;
	tmin = fmaxf(tmin, fminf(ty1, ty2));
	tmax = fminf(tmax, fmaxf(ty1, ty2));

	if (tmax >= tmin && tmin < ray.hit.intersectionDistance && tmax > 0)
		return tmin;
	else
		return fMax;

	//float tx1 = (bmin.x - ray.O.x) * ray.rD.x;
	//float tx2 = (bmax.x - ray.O.x) * ray.rD.x;
	//float tmin = min(tx1, tx2);
	//float tmax = max(tx1, tx2);

	//float ty1 = (bmin.y - ray.O.y) * ray.rD.y;
	//float ty2 = (bmax.y - ray.O.y) * ray.rD.y;
	//tmin = max(tmin, min(ty1, ty2));
	//tmax = min(tmax, max(ty1, ty2));
	//
	//float tz1 = (bmin.z - ray.O.z) * ray.rD.z;
	//float tz2 = (bmax.z - ray.O.z) * ray.rD.z;
	//tmin = max(tmin, min(tz1, tz2));
	//tmax = min(tmax, max(tz1, tz2));
	//
	//if (tmax >= tmin && tmin < ray.hit.t && tmax > 0) return tmin; else return 1e30f;
}

GLfloat BVH2D::EvaluateSAH(BVHNode& node, GLubyte axis, GLfloat pos)
{
	aabb2d leftBox, rightBox;
	GLint leftCount = 0, rightCout = 0;
	for (GLuint i = 0; i < node.obstCount; i++)
	{
		Obstacle* obst = obstacles[obstaclesIndexes[node.leftOrFirst + 1]];
		if (obst->centroid[axis] < pos)
		{
			leftCount++;
			leftBox.grow(obst->aabbMax);
			leftBox.grow(obst->aabbMin);
		}
		else
		{
			rightCout++;
			rightBox.grow(obst->aabbMax);
			rightBox.grow(obst->aabbMin);
		}
	}
	GLfloat cost = leftCount * leftBox.halfPerimeter() + rightCout * rightBox.halfPerimeter();
	return cost > 0 ? cost : fMax;
}

GLfloat BVH2D::FindBestSplitPLane(BVHNode& node, GLint& axis, GLfloat& splitPos)
{
	GLfloat bestCost = fMax;
	for (GLint a = 0; a < 2; a++) // axis
	{
		GLfloat boundsMin = fMax, boundsMax = fMin;
		for (GLuint i = 0; i < node.obstCount; i++)
		{
			Obstacle* obst = obstacles[obstaclesIndexes[node.leftOrFirst + i]];
			boundsMin = std::min(boundsMin, obst->centroid[a]);
			boundsMax = std::max(boundsMax, obst->centroid[a]);
		}
		if (boundsMin == boundsMax) continue;

		Bin bin[BINS];
		GLfloat scale = BINS / (boundsMax - boundsMin);
		for (GLuint i = 0; i < node.obstCount; i++)
		{
			Obstacle* obst = obstacles[obstaclesIndexes[node.leftOrFirst + 1]];
			GLint binIdx = std::min(BINS - 1, (GLint)((obst->centroid[a] - boundsMin) * scale));
			bin[binIdx].obstCount++;
			bin[binIdx].bounds.grow(obst->aabbMin);
			bin[binIdx].bounds.grow(obst->aabbMax);
		}

		GLfloat leftPerimeter[BINS - 1], rightPerimeter[BINS - 1];
		GLint leftCount[BINS - 1], rightCount[BINS - 1];
		aabb2d leftBox, rightBox;
		GLint leftSum = 0, rightSum = 0;
		for (GLint i = 0; i < BINS - 1; i++)
		{
			leftSum += bin[i].obstCount;
			leftCount[i] = leftSum;
			leftBox.grow(bin[i].bounds);
			leftPerimeter[i] = leftBox.halfPerimeter();
			rightSum += bin[BINS - 1 - i].obstCount;
			rightCount[BINS - 2 - i] = rightSum;
			rightBox.grow(bin[BINS - 1 - i].bounds);
			rightPerimeter[BINS - 2 - i] = rightBox.halfPerimeter();
		}

		scale = (boundsMax - boundsMin) / BINS;
		for (GLint i = 0; i < BINS - 1; i++)
		{
			GLfloat planeCost = leftCount[i] * leftPerimeter[i] + rightCount[i] * rightPerimeter[i];
			if (planeCost < bestCost)
			{
				axis = a, splitPos = boundsMin + scale * (i + 1), bestCost = planeCost;
			}
		}
	}
	return bestCost;
}
