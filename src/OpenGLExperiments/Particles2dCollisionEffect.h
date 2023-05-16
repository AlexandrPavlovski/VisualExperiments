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
	GLfloat PosXprev;
	GLfloat PosYprev;
	GLfloat AccX;
	GLfloat AccY;
	GLfloat Pressure;
	GLfloat Unused;
};


class Validator
{
public:
	void Init(Particle* particles, GLuint particlesCount, GLuint cellSize, float windowWidth, float windowHeight, GLuint phase1WorkGroupsCount, GLuint radixCountersLength)
	{
		this->particles = particles;

		this->particlesCount = particlesCount;
		this->cellsCount = particlesCount * 4;
		this->cellSize = cellSize;
		this->windowWidth = windowWidth;
		this->windowHeight = windowHeight;
		this->phase1GroupCount = phase1WorkGroupsCount;
		this->globalCountersCount = phase1WorkGroupsCount * sharedCountersLength;
		this->radixCountersLength = radixCountersLength;

		cellIds = std::vector<GLuint>(cellsCount);
		objectIds = std::vector<GLuint>(cellsCount);
		cellIdsOutput = std::vector<GLuint>(cellsCount);
		objectIdsOutput = std::vector<GLuint>(cellsCount);
		globalCounters = std::vector<GLuint>(globalCountersCount);
		totalSumms = std::vector<GLuint>(radixCountersLength);
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
		GLuint cellIdShift = pass * 4;

		for (int workGroupID = 0; workGroupID < phase1GroupCount; workGroupID++)
		{
			GLuint globalCountersOffset = workGroupID * sharedCountersLength;

			for (int localInvocationID = 0; localInvocationID < threadGroupsInWorkGroup * threadsInThreadGroup; localInvocationID++)
			{
				GLuint indexInGroup = localInvocationID % threadsInThreadGroup;
				GLuint groupIndex = localInvocationID / threadsInThreadGroup;
				GLuint cellIndexToReadFrom = (workGroupID * threadGroupsInWorkGroup + groupIndex) * elementsPerGroup + indexInGroup;
				GLuint counterIndexOffset = groupIndex * radixCountersLength;

				for (int i = 0; i < elementsPerGroup; i += threadsInThreadGroup)
				{
					if (cellIndexToReadFrom + i < cellsCount)
					{
						GLuint cellId = cellIds[cellIndexToReadFrom + i];
						GLuint radix = (cellId >> cellIdShift) & 15;
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
					for (int b = 0; b < globalCountersCount; b += radixCountersLength)
					{
						for (int c = 0; c < radixCountersLength; c++)
						{
							MyFile << globalCountersFromGPU[b + c] << "\t";
						}
						MyFile << "\r";
					}
					MyFile.close();

					std::ofstream MyFile2("c:/ValidateSortPhase1 globalCounters.txt");
					for (int b = 0; b < globalCountersCount; b += radixCountersLength)
					{
						for (int c = 0; c < radixCountersLength; c++)
						{
							MyFile2 << globalCounters[b + c] << "\t";
						}
						MyFile2 << "\r";
					}
					MyFile2.close();
				}
			}
		}
	}

	void ValidateSortPhase2(GLuint threadGroupsTotal, GLuint* globalCountersFromGPU, GLuint* totalSummsFromGPU)
	{
		for (int radix = 0; radix < radixCountersLength; radix++)
		{
			GLuint summ = 0;
			for (int threadGroup = 0; threadGroup < threadGroupsTotal; threadGroup++)
			{
				GLuint globalCounterIndex = threadGroup * radixCountersLength + radix;

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
					for (int b = 0; b < globalCountersCount; b+= radixCountersLength)
					{
						for (int c = 0; c < radixCountersLength; c++)
						{
							MyFile << globalCountersFromGPU[b + c] << "\t";
						}
						MyFile << "\r";
					}
					MyFile.close();
				}
			}
		}
		for (int i = 0; i < radixCountersLength; i++)
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
		for (int i = 0; i < radixCountersLength; i++)
		{
			GLuint counter = totalSumms[i];
			totalSumms[i] = summ;
			summ += counter;
		}

		GLuint cellIdShift = pass * 4;

		for (int workGroupID = 0; workGroupID < phase1GroupCount; workGroupID++)
		{
			std::vector<GLuint> sharedCounters = std::vector<GLuint>(sharedCountersLength);
			for (int i = 0; i < sharedCountersLength; i++)
			{
				sharedCounters[i] = globalCounters[workGroupID * sharedCountersLength + i] + totalSumms[i % radixCountersLength];
			}

			for (int localInvocationID = 0; localInvocationID < threadGroupsInWorkGroup * threadsInThreadGroup; localInvocationID++)
			{
				GLuint indexInGroup = localInvocationID % threadsInThreadGroup;
				GLuint groupIndex = localInvocationID / (float)threadsInThreadGroup;
				GLuint cellIndexToReadFrom = (workGroupID * threadGroupsInWorkGroup+ groupIndex) * elementsPerGroup + indexInGroup;
				GLuint counterIndexOffset = groupIndex * radixCountersLength;

				for (int i = 0; i < elementsPerGroup; i += threadsInThreadGroup)
				{
					if (cellIndexToReadFrom + i < cellsCount)
					{
						GLuint cellId = cellIds[cellIndexToReadFrom + i];
						GLuint objectId = objectIds[cellIndexToReadFrom + i];
						GLuint radix = (cellId >> cellIdShift) & 15;
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

	GLuint particlesCount, cellsCount, cellSize, phase1GroupCount, globalCountersCount, radixCountersLength;
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

	GLuint radixCountersLength = 16;
	GLuint sharedCountersLength = 12032; // 12288 is maximum on my laptop's 3060, but in phase 3 need some additional shared memory for total summs counting
	GLfloat maxThreadsInWorkGroup = 1024.0; // 1024 is maximum threads per work group on my laptop's 3060
	GLuint threadGroupsInWorkGroup = sharedCountersLength / radixCountersLength;
	GLuint threadsInThreadGroup = ceil(threadGroupsInWorkGroup / maxThreadsInWorkGroup); // 16 is maximum for inter-thread synchronization to work

	GLuint maxWorkGroupCount = 0;
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
		bool IsNBodyGravity;
	};

	struct RuntimeParams
	{
		GLfloat ForceScale;
		GLfloat VelocityDamping;
		GLfloat TimeScale;
		GLfloat particleSize;
		GLfloat cellSize;
		GLint substeps;
	};

	struct ShaderParams
	{
		ShaderParam nBody;
		ShaderParam particlesCount;
		ShaderParam cellIdsLength;
		ShaderParam threadsInWorkGroup;
		ShaderParam threadGroupsInWorkGroup;
		ShaderParam radixCountersLength;
		ShaderParam threadsInThreadGroup;
		ShaderParam elementsPerGroup;
		ShaderParam threadGroupsTotal;
		ShaderParam phase2Iterations;
		ShaderParam bindingCellIds1;
		ShaderParam bindingCellIds2;
		ShaderParam cells1;
		ShaderParam cells2;
		ShaderParam bindingCellIdsInput;
		ShaderParam bindingCellIdsOutput;
		ShaderParam bufferCellIdsInput;
		ShaderParam bufferCellIdsOutput;
		ShaderParam bindingObjectIdsInput;
		ShaderParam bindingObjectIdsOutput;
		ShaderParam bufferObjectIdsInput;
		ShaderParam bufferObjectIdsOutput;
	};

	struct RadixSortPhase
	{
		GLuint Phase1;
		GLuint Phase2;
		GLuint Phase3;
	};

	StartupParams startupParams;
	RuntimeParams runtimeParams;

	GLint currentParticlesCount = 0;
	GLint currentCellsCount = 0;
	bool isPaused = false;
	bool isAdvanceOneFrame = false;
	bool isDebug = false;

	GLint frameCount = 0;

	const static GLuint totalSortPasses = 4;

	GLuint vao = 0, ssboParticles = 0, ssboObjectId = 0, ssboCellId = 0, ssboGlobalCounters = 0, ssboCollisionList = 0, ssboMisc = 0;
	GLuint fillCellIdAndObjectIdArraysCompShaderProgram = 0,
		findCollisionCellsCompShaderProgram = 0,
		resolveCollisionsCompShaderProgram = 0,
		gridShaderProgram = 0;
	RadixSortPhase RadixSortPasses[totalSortPasses] = { 0 };


	bool isManualAttractorControlEnabled = false;
	GLuint phase1GroupCount = 0;

	// ------ mouse ------ //
	bool isLeftMouseBtnDown = false;
	bool isLeftMouseBtnPressed = false;
	bool isLeftMouseBtnReleased = false;
	GLdouble cursorPosX = 0.0, cursorPosY = 0.0;
	// ------------------- //

	void initBuffers();
	void initParticles();
	ShaderParams initShaderParams();
	void initRadixSortShaderProgramms(ShaderParams shaderParams);

	void createComputeShaderProgram(GLuint& compShaderProgram, const char* shaderFilePath, std::vector<ShaderParam> shaderParams = std::vector<ShaderParam>());
	void cleanup();

	template< typename T >
	T* readFromBuffer(int elemCount, GLuint ssbo);

	
	Particle* particlesPrev = 0;

	// === for performance profiling ===
	static const int queriesSize = 4;
	static const int queriesForRadixSortSize = totalSortPasses * 3;
	double accumulatedTimeCpu = 0;
	double accumulatedTimeGpu[queriesSize] = { 0 }, accumulatedTimeGpuRadixSort[queriesForRadixSortSize] = { 0 };
	float averageFrameTimeCpu = 0;
	double averageFrameTimeGpu[queriesSize] = { 0 }, averageFrameTimeGpuRadixSort[queriesForRadixSortSize] = { 0 };
	// =================================
};
