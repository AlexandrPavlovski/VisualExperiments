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
	//GLfloat Pressure;
	//GLfloat Unused;
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
		ShaderParam elementsPerThread;
		ShaderParam threadGroupsTotal;
		ShaderParam phase2Iterations;
		ShaderParam sharedCountersLength;
		ShaderParam bitMask;
		ShaderParam cellsPerThread;

		ShaderParam bindingCellIds;
		ShaderParam cells;

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

	// core parameters for the whole simulation
	static const GLuint cellIdBits = 16;
	static const GLuint bitsPerSortPass = 8;
	GLuint sharedCountersLength = 12032; // 12288 is maximum on my laptop's 3060, but in phase 3 need some additional shared memory for total summs counting
	GLfloat maxThreadsInWorkGroup = 1024.0; // 1024 is maximum threads per work group on my laptop's 3060
	GLuint elementsPerThread = 32; // number of cells each thread will process in phases 1 and 3
	GLuint cellsPerThread = 32; // number of cells each thread will process when building collision cell list

	static const GLuint totalSortPasses = cellIdBits / bitsPerSortPass;
	GLuint maxNumberOfCells = pow(2, cellIdBits);
	GLuint radixCountersLength = pow(2, bitsPerSortPass);
	GLuint bitMask = radixCountersLength - 1;
	GLuint threadGroupsInWorkGroup = sharedCountersLength / radixCountersLength;
	GLuint threadsInThreadGroup = 16; //TODO std::max((int)floor(maxThreadsInWorkGroup / threadGroupsInWorkGroup), 16); // 16 is maximum for inter-thread synchronization to work
	GLuint threadsInWorkGroupInPhases1And3 = threadGroupsInWorkGroup * threadsInThreadGroup;

	GLuint workGroupsCountInFindCollisionCellsPhase = 0;
	GLuint maxWorkGroupCount = 0;
	GLuint elementsPerGroup = 0;
	GLuint threadGroupsTotal = 0;
	GLuint phase2Iterations = 0;

	std::vector<Particle> particles;

	// temp vars
	GLuint buffer = 0;
	GLuint buffer5 = 0;
	GLuint buffer6 = 0;
	GLuint buffer7 = 0;
	GLuint buffer8 = 0;
	GLuint bufferTest = 0;

	GLint currentParticlesCount = 0;
	GLint currentCellsCount = 0;
	bool isPaused = false;
	bool isAdvanceOneFrame = false;
	bool isDebug = false;

	GLint frameCount = 0;
	GLint frameCount2 = 0;


	GLuint vao = 0, ssboParticles = 0, ssboObjectId = 0, ssboCellId = 0, ssboGlobalCounters = 0,
		ssboCollisionList = 0, ssboMisc = 0, ssboCollCellsFound = 0, ssboCellIdsLookup = 0;
	GLuint fillCellIdAndObjectIdArraysCompShaderProgram = 0,
		findCollisionCellsCompShaderProgram = 0,
		compactCollisionCellsCompShaderProgram = 0,
		resolveCollisionsCompShaderProgram = 0,
		mouseInteractionsCompShaderProgram = 0,
		gridShaderProgram = 0;
	std::vector<RadixSortPhase> RadixSortPasses = std::vector<RadixSortPhase>(totalSortPasses);


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
	void createSsbo(GLuint* buff, GLuint index, GLsizeiptr size, const void* data, GLenum usage);
	void cleanup();

	template< typename T >
	T* readFromBuffer(int elemCount, GLuint ssbo);
	template< typename T >
	T* readFromBuffer2(int elemCount, GLuint ssbo);

	
	Particle* particlesPrev = 0;

	// === for validation ===
	void initValidation(Particle* particles);
	void validateFilledArrays(GLuint* cellsFromGPU, GLuint* objectsFromGPU);
	void validateSortPhase1(GLuint pass, GLuint* globalCountersFromGPU);
	void validateSortPhase2(GLuint* globalCountersFromGPU, GLuint* totalSummsFromGPU);
	void validateSortPhase3(GLuint pass, GLuint* cellsFromGPU, GLuint* objectsFromGPU);
	void validateFindCollisionCells(GLuint* collsionCellsFromGPU, GLuint* collCellsFoundPerWorkGroupFromGPU, GLuint* lookupFromGPU);
	void clearValidation();

	GLuint vGlobalCountersCount;

	Particle* vParticles;
	std::vector<GLuint> vCellIds;
	std::vector<GLuint> vObjectIds;
	std::vector<GLuint> vCellIdsOutput;
	std::vector<GLuint> vObjectIdsOutput;
	std::vector<GLuint> vGlobalCounters;
	std::vector<GLuint> vTotalSumms;
	std::vector<GLuint> vCollisionCells;
	// ======================


	// === for performance profiling ===
	static const int framesToAvegare = 100;
	int currentlyAveragedFrames = 0;
	static const int queriesSize = 5;
	static const int queriesForRadixSortSize = totalSortPasses * 3;
	double accumulatedTimeCpu = 0;
	double accumulatedTimeGpu[queriesSize] = { 0 };
	std::vector<double> accumulatedTimeGpuRadixSort = std::vector<double>(queriesForRadixSortSize);
	float averageFrameTimeCpu = 0;
	double averageFrameTimeGpu[queriesSize] = { 0 };
	std::vector<double> averageFrameTimeGpuRadixSort = std::vector<double>(queriesForRadixSortSize);
	// =================================
};
