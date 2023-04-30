#pragma once

#include "AbstractEffect.h"

#include <vector>
#include <iostream>
#include <fstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>


struct Particle
{
	GLfloat PosX;
	GLfloat PosY;
	GLfloat VelX;
	GLfloat VelY;
	GLfloat AccX;
	GLfloat AccY;
	GLfloat Pressure;
	GLfloat Unused;
};


class Validator
{
public:
	void Init(Particle* particles, GLuint particlesCount, GLuint cellSize, float windowWidth, float windowHeight, GLuint phase1WorkGroupsCount)
	{
		this->particles = particles;

		this->particlesCount = particlesCount;
		this->cellsCount = particlesCount * 4;
		this->cellSize = cellSize;
		this->windowWidth = windowWidth;
		this->windowHeight = windowHeight;
		this->phase1WorkGroupsCount = phase1WorkGroupsCount;
		this->globalCountersCount = phase1WorkGroupsCount * sharedCountersLength;

		cellIds = std::vector<GLuint>(cellsCount);
		objectIds = std::vector<GLuint>(cellsCount);
		cellIdsOutput = std::vector<GLuint>(cellsCount);
		objectIdsOutput = std::vector<GLuint>(cellsCount);
		globalCounters = std::vector<GLuint>(globalCountersCount);
		totalSumms = std::vector<GLuint>(256);
	}

	void ValidateFilledArrays(GLuint* cellsFromGPU, GLuint* objectsFromGPU)
	{
		GLuint gridWidth = windowWidth / cellSize + 2;

		for (int i = 0; i < particlesCount; i++)
		{
			Particle p = particles[i];

			double gridPosX, gridPosY;
			double cellPosX = modf(p.PosX / cellSize, &gridPosX);
			double cellPosY = modf(p.PosY / cellSize, &gridPosY);
			gridPosX++;
			gridPosY++;

			GLuint hCellId = gridPosX + gridPosY * gridWidth;

			GLuint pCellIdsX, pCellIdsY, pCellIdsZ;
			if (cellPosX > 0.5)
			{
				pCellIdsX = hCellId + 1;
				if (cellPosY > 0.5)
				{
					pCellIdsY = hCellId + gridWidth;
					pCellIdsZ = hCellId + gridWidth + 1;
				}
				else
				{
					pCellIdsY = hCellId - gridWidth;
					pCellIdsZ = hCellId - gridWidth + 1;
				}
			}
			else
			{
				pCellIdsX = hCellId - 1;
				if (cellPosY > 0.5)
				{
					pCellIdsY = hCellId + gridWidth;
					pCellIdsZ = hCellId + gridWidth - 1;
				}
				else
				{
					pCellIdsY = hCellId - gridWidth;
					pCellIdsZ = hCellId - gridWidth - 1;
				}
			}

			cellIds[i] = hCellId;
			cellIds[i + particlesCount] = pCellIdsX;
			cellIds[i + particlesCount * 2] = pCellIdsY;
			cellIds[i + particlesCount * 3] = pCellIdsZ;

			objectIds[i] = i | 0x80000000;
			objectIds[i + particlesCount] = i;
			objectIds[i + particlesCount * 2] = i;
			objectIds[i + particlesCount * 3] = i;
		}

		for (int i = 0; i < particlesCount; i++)
		{
			if (cellsFromGPU[i] != cellIds[i])
			{
				__debugbreak();
			}
			if (objectsFromGPU[i] != objectIds[i])
			{
				__debugbreak();
			}
		}
	}

	void ValidateSortPhase1(GLuint pass, GLuint threadsInWorkGroup, GLuint threadGroupsInWorkGroup, GLuint threadsInThreadGroup, GLuint elementsPerGroup, GLuint* globalCountersFromGPU)
	{
		GLuint cellIdShift = pass == 0 ? 0 : 8;

		for (int workGroupID = 0; workGroupID < phase1WorkGroupsCount; workGroupID++)
		{
			GLuint globalCountersOffset = workGroupID * sharedCountersLength;

			for (int localInvocationID = 0; localInvocationID < threadsInWorkGroup; localInvocationID++)
			{
				GLuint indexInGroup = localInvocationID % threadsInThreadGroup;
				GLuint groupIndex = localInvocationID / threadsInThreadGroup;
				GLuint cellIndexToReadFrom = (workGroupID * threadGroupsInWorkGroup + groupIndex) * elementsPerGroup + indexInGroup;
				GLuint counterIndexOffset = groupIndex * 256;

				for (int i = 0; i < elementsPerGroup; i += threadsInThreadGroup)
				{
					if (cellIndexToReadFrom + i < cellsCount)
					{
						GLuint cellId = cellIds[cellIndexToReadFrom + i];
						GLuint radix = (cellId >> cellIdShift) & 255;
						GLuint indexInSharedCounters = counterIndexOffset + radix;

						globalCounters[indexInSharedCounters + globalCountersOffset]++;
					}
				}
			}
		}

		for (int i = 0; i < globalCountersCount; i++)
		{
			if (globalCountersFromGPU[i] != globalCounters[i])
			{
				__debugbreak();
				if (i == -1)
				{
					std::ofstream MyFile("c:/ValidateSortPhase1 globalCountersFromGPU.txt");
					for (int b = 0; b < globalCountersCount; b += 256)
					{
						for (int c = 0; c < 256; c++)
						{
							MyFile << globalCountersFromGPU[b + c] << "\t";
						}
						MyFile << "\r";
					}
					MyFile.close();
				}
			}
		}
	}

	void ValidateSortPhase2(GLuint threadGroupsTotal, GLuint* globalCountersFromGPU, GLuint* totalSummsFromGPU)
	{
		for (int radix = 0; radix < 256; radix++)
		{
			GLuint summ = 0;
			for (int threadGroup = 0; threadGroup < threadGroupsTotal; threadGroup++)
			{
				GLuint globalCounterIndex = threadGroup * 256 + radix;

				GLuint counter = globalCounters[globalCounterIndex];
				globalCounters[globalCounterIndex] = summ;
				summ += counter;
			}
			totalSumms[radix] = summ;
		}

		for (int i = 0; i < globalCountersCount; i++)
		{
			if (globalCountersFromGPU[i] != globalCounters[i])
			{
				__debugbreak();
				if (i == -1)
				{
					std::ofstream MyFile("c:/ValidateSortPhase2 globalCountersFromGPU.txt");
					for (int b = 0; b < globalCountersCount; b+=256)
					{
						for (int c = 0; c < 256; c++)
						{
							MyFile << globalCountersFromGPU[b + c] << "\t";
						}
						MyFile << "\r";
					}
					MyFile.close();
				}
			}
		}
		for (int i = 0; i < 256; i++)
		{
			if (totalSummsFromGPU[i] != totalSumms[i])
			{
				__debugbreak();
			}
		}
	}

	void ValidateSortPhase3(
		GLuint pass,
		GLuint threadsInWorkGroup,
		GLuint threadGroupsInWorkGroup,
		GLuint threadsInThreadGroup,
		GLuint elementsPerGroup,
		GLuint* cellsFromGPU,
		GLuint* objectsFromGPU)
	{
		GLuint summ = 0;
		for (int i = 0; i < 256; i++)
		{
			GLuint counter = totalSumms[i];
			totalSumms[i] = summ;
			summ += counter;
		}

		GLuint sharedCountersLength = 12032;
		GLuint cellIdShift = pass == 0 ? 0 : 8;

		for (int workGroupID = 0; workGroupID < phase1WorkGroupsCount; workGroupID++)
		{
			std::vector<GLuint> sharedCounters = std::vector<GLuint>(sharedCountersLength);
			for (int i = 0; i < sharedCountersLength; i++)
			{
				sharedCounters[i] = globalCounters[workGroupID * sharedCountersLength + i] + totalSumms[i % 256];
			}

			for (int localInvocationID = 0; localInvocationID < threadsInWorkGroup; localInvocationID++)
			{
				GLuint indexInGroup = localInvocationID % threadsInThreadGroup;
				GLuint groupIndex = localInvocationID / (float)threadsInThreadGroup;
				GLuint cellIndexToReadFrom = (workGroupID * threadGroupsInWorkGroup+ groupIndex) * elementsPerGroup + indexInGroup;
				GLuint counterIndexOffset = groupIndex * 256;

				for (int i = 0; i < elementsPerGroup; i += threadsInThreadGroup)
				{
					if (cellIndexToReadFrom + i < cellsCount)
					{
						GLuint cellId = cellIds[cellIndexToReadFrom + i];
						GLuint objectId = objectIds[cellIndexToReadFrom + i];
						GLuint radix = (cellId >> cellIdShift) & 255;
						GLuint indexInSharedCounters = counterIndexOffset + radix;

						GLuint offset = sharedCounters[indexInSharedCounters];

						cellIdsOutput[offset] = cellId;
						objectIdsOutput[offset] = objectId;

						sharedCounters[indexInSharedCounters]++;
					}
				}
			}

			sharedCounters.clear();
		}

		if (cellsFromGPU != nullptr && objectsFromGPU != nullptr)
		{
			for (int i = 0; i < cellsCount; i++)
			{
				if (cellsFromGPU[i] != cellIdsOutput[i])
				{
					__debugbreak();
				}
				if (objectsFromGPU[i] != objectIdsOutput[i])
				{
					__debugbreak();
				}
			}
		}

		cellIds.clear();
		objectIds.clear();
		cellIds.shrink_to_fit();
		objectIds.shrink_to_fit();

		cellIds = cellIdsOutput;
		objectIds = objectIdsOutput;

		cellIdsOutput.clear();
		objectIdsOutput.clear();
		globalCounters.clear();

		globalCounters = std::vector<GLuint>(globalCountersCount);
		cellIdsOutput = std::vector<GLuint>(cellsCount);
		objectIdsOutput = std::vector<GLuint>(cellsCount);
	}

	void Clear()
	{
		cellIds.clear();
		objectIds.clear();
		globalCounters.clear();
		totalSumms.clear();
		cellIdsOutput.clear();
		objectIdsOutput.clear();

		cellIds.shrink_to_fit();
		objectIds.shrink_to_fit();
		globalCounters.shrink_to_fit();
		totalSumms.shrink_to_fit();
		cellIdsOutput.shrink_to_fit();
		objectIdsOutput.shrink_to_fit();
	}

	GLuint sharedCountersLength = 12032;

	GLuint particlesCount, cellsCount, cellSize, phase1WorkGroupsCount, globalCountersCount;
	float windowWidth, windowHeight;

	Particle* particles;
	std::vector<GLuint> cellIds;
	std::vector<GLuint> objectIds;
	std::vector<GLuint> cellIdsOutput;
	std::vector<GLuint> objectIdsOutput;
	std::vector<GLuint> globalCounters;
	std::vector<GLuint> totalSumms;
};



class Particles2dCollisionEffect :public AbstractEffect
{
public:
	Particles2dCollisionEffect(GLFWwindow* window);
	virtual ~Particles2dCollisionEffect();

	virtual void initialize();
	virtual void draw(GLdouble deltaTime);
	virtual void drawGUI();
	virtual void restart();

	virtual void keyCallback(int key, int scancode, int action, int mode);
	virtual void mouseButtonCallback(int button, int action, int mods);

	GLuint radixCountersLength = 0;
	GLuint sharedCountersLength = 0;
	GLuint maxWorkGroupCount = 0;
	GLuint threadGroupsInWorkGroup = 0;
	GLuint threadsInThreadGroup = 0;
	GLfloat threadsInWorkGroup = 0;
	GLuint elementsPerThread = 0;
	GLuint elementsPerGroup = 0;
	GLuint threadGroupsTotal = 0;

	std::vector<Particle> particles;

	// temp vars
	GLuint buffer = 0;
	GLuint buffer5 = 0;
	GLuint buffer6 = 0;
	GLuint buffer7 = 0;
	GLuint buffer8 = 0;
	GLuint bufferTest = 0;
	GLuint frame = 0;
	Validator* validator = nullptr;

private:
	struct StartupParams
	{
		int ParticlesCount;
	};

	struct RuntimeParams
	{
		GLdouble AttractorX;
		GLdouble AttractorY;
		GLfloat ForceScale;
		GLfloat VelocityDamping;
		GLfloat MinDistanceToAttractor;
		GLfloat TimeScale;
		GLfloat Color[4];
		GLfloat particleSize;
		GLfloat cellSize;
	};

	StartupParams startupParams;
	RuntimeParams runtimeParams;

	GLint currentParticlesCount = 0;
	GLint currentCellsCount = 0;
	bool isPaused = false;
	bool isAdvanceOneFrame = false;
	bool isDebug = true;

	GLint frameCount = 0;

	GLuint vao = 0, ssboParticles = 0, ssboObjectId = 0, ssboCellId = 0, ssboGlobalCounters = 0, ssboCollisionList = 0, ssboMisc = 0;
	GLuint fillCellIdAndObjectIdArraysCompShaderProgram = 0,
		radixPhase1Pass1CompShaderProgram = 0, radixPhase1Pass2CompShaderProgram = 0,
		radixPhase2CompShaderProgram = 0,
		radixPhase3Pass1CompShaderProgram = 0, radixPhase3Pass2CompShaderProgram = 0,
		findCollisionCellsCompShaderProgram = 0,
		resolveCollisionsCompShaderProgram = 0,
		gridShaderProgram = 0;

	bool isManualAttractorControlEnabled = false;
	GLuint phase1GroupCount = 0;

	// ------ mouse ------ //
	bool isLeftMouseBtnDown = false;
	bool isLeftMouseBtnPressed = false;
	bool isLeftMouseBtnReleased = false;
	GLdouble cursorPosX = 0.0, cursorPosY = 0.0;
	// ------------------- //

	void createComputeShaderProgram(GLuint& compShaderProgram, const char* shaderFilePath, std::vector<ShaderParams> shaderParams = std::vector<ShaderParams>());

	template< typename T >
	T* readFromBuffer(int elemCount, GLuint ssbo);

	void cleanup();
};
