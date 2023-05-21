// implementation of this article
// https://developer.nvidia.com/gpugems/gpugems3/part-v-physics-simulation/chapter-32-broad-phase-collision-detection-cuda

//#define VLAIDATE
//#define MISC_TESTS
#define PROFILE

#include <chrono>
#include <iostream>
#include "Particles2dCollisionEffect.h"

Particles2dCollisionEffect::Particles2dCollisionEffect(GLFWwindow* window)
	:AbstractEffect(window)
{
	computeShaderFilePath = "Particles2dCollisionEffect.comp";
	vertexShaderFilePath = "Particles2dCollisionEffect.vert";
	fragmentShaderFilePath = "Particles2dCollisionEffect.frag";

	startupParams = {};
	startupParams.ParticlesCount = 300000;
	startupParams.IsNBodyGravity = false;

	runtimeParams = {};
	runtimeParams.ForceScale = 1.0;
	runtimeParams.VelocityDamping = 0.999;
	runtimeParams.TimeScale = 1.000;
	runtimeParams.particleSize = 2.0;
	runtimeParams.cellSize = 4.0;//ceil(sqrt(windowWidth * windowHeight / pow(2, cellIdBits)));
	runtimeParams.substeps = 1;
}

Particles2dCollisionEffect::~Particles2dCollisionEffect()
{
	cleanup();
}

void Particles2dCollisionEffect::initialize()
{
	currentParticlesCount = startupParams.ParticlesCount;
	currentCellsCount = startupParams.ParticlesCount * 4;

	phase1GroupCount = ceil(currentCellsCount / (float)threadsInWorkGroupInPhases1And3);

	elementsPerThread = ceil(currentCellsCount / (phase1GroupCount * (float)threadsInWorkGroupInPhases1And3));
	elementsPerGroup = threadsInThreadGroup * elementsPerThread;
	threadGroupsTotal = ceil(currentCellsCount / (double)elementsPerGroup);

	phase2Iterations = ceil(threadGroupsTotal / maxThreadsInWorkGroup / 2);

	initParticles();
	initBuffers();

	particles.clear();

	ShaderParams shaderParams = initShaderParams();

	std::vector<ShaderParam> vertShaderParams
	{
		shaderParams.particlesCount,
		shaderParams.nBody
	};
	std::vector<ShaderParam> fragShaderParams
	{
		shaderParams.particlesCount
	};
	GLint newShaderProgram = createShaderProgramFromFiles(vertShaderParams, fragShaderParams);
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}
	shaderProgram = newShaderProgram;

	newShaderProgram = createShaderProgramFromFiles("Grid.vert", "Grid.frag", std::vector<ShaderParam>(), std::vector<ShaderParam>());
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}
	gridShaderProgram = newShaderProgram;
	
	std::vector<ShaderParam> findCollisionCellsShaderParams
	{
		shaderParams.cellIdsLength
	};
	std::vector<ShaderParam> ResolveCollisionsShaderParams
	{
	};

	createComputeShaderProgram(fillCellIdAndObjectIdArraysCompShaderProgram, "FillCellIdAndObjectIdArrays.comp", fragShaderParams);
	createComputeShaderProgram(findCollisionCellsCompShaderProgram, "FindCollisionCells.comp", findCollisionCellsShaderParams);
	createComputeShaderProgram(resolveCollisionsCompShaderProgram, "ResolveCollisions.comp", ResolveCollisionsShaderParams);

	initRadixSortShaderProgramms(shaderParams);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef MISC_TESTS
particlesPrev = new Particle[currentParticlesCount];
#endif
}

void Particles2dCollisionEffect::initParticles()
{
	srand(50);

	particles = std::vector<Particle>(currentParticlesCount);
	for (int i = 0; i < currentParticlesCount; i++)
	{
		particles[i].PosX = random(101.0, 1002.0);
		particles[i].PosY = random(101.0, 702.0);

		particles[i].PosXprev = particles[i].PosX;
		particles[i].PosYprev = particles[i].PosY;


		//particles[i].VelX = random(-3.0, 3.0);
		//particles[i].VelY = random(-3.0, 3.0);
	}
}

void Particles2dCollisionEffect::initBuffers()
{
	GLuint vaoCellId = 0;
	GLuint vaoObjectId = 0;
	GLuint vaoGlobalCounters = 0;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ssboParticles);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboParticles);
	glBufferData(GL_SHADER_STORAGE_BUFFER, currentParticlesCount * sizeof(Particle), &particles[0], GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	//glGenVertexArrays(1, &vaoCellId);
	//glBindVertexArray(vaoCellId);

	glGenBuffers(1, &ssboCellId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCellId);
	glBufferData(GL_SHADER_STORAGE_BUFFER, currentCellsCount * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	//glGenVertexArrays(1, &vaoObjectId);
	//glBindVertexArray(vaoObjectId);

	glGenBuffers(1, &ssboObjectId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboObjectId);
	glBufferData(GL_SHADER_STORAGE_BUFFER, currentCellsCount * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboObjectId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	//glGenVertexArrays(1, &vaoGlobalCounters);
	//glBindVertexArray(vaoGlobalCounters);

	glGenBuffers(1, &ssboGlobalCounters);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboGlobalCounters);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sharedCountersLength * phase1GroupCount * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboGlobalCounters);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glGenBuffers(1, &buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, radixCountersLength * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &buffer5);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buffer5);
	glBufferData(GL_SHADER_STORAGE_BUFFER, currentCellsCount * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &buffer6);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buffer6);
	glBufferData(GL_SHADER_STORAGE_BUFFER, currentCellsCount * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &buffer7);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, buffer7);
	glBufferData(GL_SHADER_STORAGE_BUFFER, currentCellsCount * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &buffer8);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, buffer8);
	glBufferData(GL_SHADER_STORAGE_BUFFER, currentCellsCount * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &bufferTest);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, bufferTest);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 20000 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &ssboCollisionList);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssboCollisionList);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 777 * sizeof(GLint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &ssboMisc);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, ssboMisc);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

Particles2dCollisionEffect::ShaderParams Particles2dCollisionEffect::initShaderParams()
{
	ShaderParams shaderParams{};

	shaderParams.nBody =                   { "#define nBody 0",                    "#define nBody "                   + std::to_string(startupParams.IsNBodyGravity ? 1 : 0) };
	shaderParams.particlesCount =          { "#define particlesCount 0",           "#define particlesCount "          + std::to_string(currentParticlesCount) };
	shaderParams.cellIdsLength =           { "#define cellIdsLength 0",            "#define cellIdsLength "           + std::to_string(currentCellsCount) };
	shaderParams.threadsInWorkGroup =      { "#define threadsInWorkGroup 1",       "#define threadsInWorkGroup "      + std::to_string(threadsInWorkGroupInPhases1And3) };
	shaderParams.threadGroupsInWorkGroup = { "#define threadGroupsInWorkGroup 1",  "#define threadGroupsInWorkGroup " + std::to_string(threadGroupsInWorkGroup) };
	shaderParams.radixCountersLength =     { "#define radixCountersLength 1",      "#define radixCountersLength "     + std::to_string(radixCountersLength) };
	shaderParams.threadsInThreadGroup =    { "#define threadsInThreadGroup 0",     "#define threadsInThreadGroup "    + std::to_string(threadsInThreadGroup) };
	shaderParams.elementsPerGroup =        { "#define elementsPerGroup 0",         "#define elementsPerGroup "        + std::to_string(elementsPerGroup) };
	shaderParams.threadGroupsTotal =       { "#define threadGroupsTotal 1",        "#define threadGroupsTotal "       + std::to_string(threadGroupsTotal) };
	shaderParams.phase2Iterations =        { "#define iterations 0",               "#define iterations "              + std::to_string(phase2Iterations) };
	shaderParams.sharedCountersLength =    { "#define sharedCountersLength 1",     "#define sharedCountersLength "    + std::to_string(sharedCountersLength) };
	shaderParams.bitMask =                 { "#define bitMask 1",                  "#define bitMask "                 + std::to_string(bitMask) };

	shaderParams.bindingCellIds =          { "#define bindingCellIds 1",           "#define bindingCellIds 5" };
	shaderParams.cells =                   { "#define cells cells1",               "#define cells cells2" };
	
	shaderParams.bindingCellIdsInput =     { "#define bindingCellIdsInput 1",      "#define bindingCellIdsInput 5" };
	shaderParams.bindingCellIdsOutput =    { "#define bindingCellIdsOutput 5",     "#define bindingCellIdsOutput 1" };
	shaderParams.bufferCellIdsInput =      { "#define bufferCellIdsInput cells1",  "#define bufferCellIdsInput cells2" };
	shaderParams.bufferCellIdsOutput =     { "#define bufferCellIdsOutput cells2", "#define bufferCellIdsOutput cells1" };
	
	shaderParams.bindingObjectIdsInput =   { "#define bindingObjectIdsInput 2",        "#define bindingObjectIdsInput 7" };
	shaderParams.bindingObjectIdsOutput =  { "#define bindingObjectIdsOutput 7",       "#define bindingObjectIdsOutput 2" };
	shaderParams.bufferObjectIdsInput =    { "#define bufferObjectIdsInput objects1",  "#define bufferObjectIdsInput objects2" };
	shaderParams.bufferObjectIdsOutput =   { "#define bufferObjectIdsOutput objects2", "#define bufferObjectIdsOutput objects1" };

	return shaderParams;
}

void Particles2dCollisionEffect::initRadixSortShaderProgramms(ShaderParams shaderParams)
{
	std::vector<ShaderParam> phase1EvenPassShaderParams
	{
		shaderParams.cellIdsLength,
		shaderParams.radixCountersLength,
		shaderParams.threadsInWorkGroup,
		shaderParams.threadGroupsInWorkGroup,
		shaderParams.threadsInThreadGroup,
		shaderParams.elementsPerGroup,
		shaderParams.sharedCountersLength,
		shaderParams.bitMask
	};
	std::vector<ShaderParam> phase1OddPassShaderParams = phase1EvenPassShaderParams;
	phase1OddPassShaderParams.push_back(shaderParams.bindingCellIds);
	phase1OddPassShaderParams.push_back(shaderParams.cells);

	std::vector<ShaderParam> phase2ShaderParams
	{
		shaderParams.radixCountersLength,
		shaderParams.threadGroupsTotal,
		//shaderParams.threadsInWorkGroup,
		shaderParams.phase2Iterations
	};

	std::vector<ShaderParam> phase3EvenPassShaderParams
	{
		shaderParams.cellIdsLength,
		shaderParams.threadsInWorkGroup,
		shaderParams.threadGroupsInWorkGroup,
		shaderParams.radixCountersLength,
		shaderParams.threadsInThreadGroup,
		shaderParams.elementsPerGroup,
		shaderParams.sharedCountersLength,
		shaderParams.bitMask
	};
	std::vector<ShaderParam> phase3OddPassShaderParams = phase3EvenPassShaderParams;
	phase3OddPassShaderParams.push_back(shaderParams.bindingCellIdsInput);
	phase3OddPassShaderParams.push_back(shaderParams.bindingCellIdsOutput);
	phase3OddPassShaderParams.push_back(shaderParams.bufferCellIdsInput);
	phase3OddPassShaderParams.push_back(shaderParams.bufferCellIdsOutput);
	phase3OddPassShaderParams.push_back(shaderParams.bindingObjectIdsInput);
	phase3OddPassShaderParams.push_back(shaderParams.bindingObjectIdsOutput);
	phase3OddPassShaderParams.push_back(shaderParams.bufferObjectIdsInput);
	phase3OddPassShaderParams.push_back(shaderParams.bufferObjectIdsOutput);


	for (int i = 0; i < totalSortPasses; i++)
	{
		std::vector<ShaderParam> phase1ShaderParams = phase1EvenPassShaderParams,
								 phase3ShaderParams = phase3EvenPassShaderParams;
		if (i % 2 != 0)
		{
			phase1ShaderParams = phase1OddPassShaderParams;
			phase3ShaderParams = phase3OddPassShaderParams;
		}

		createComputeShaderProgram(RadixSortPasses[i].Phase1, "RadixSortPhase1.comp", phase1ShaderParams);
		createComputeShaderProgram(RadixSortPasses[i].Phase2, "RadixSortPhase2.comp", phase2ShaderParams);
		createComputeShaderProgram(RadixSortPasses[i].Phase3, "RadixSortPhase3.comp", phase3ShaderParams);
	}
}


void Particles2dCollisionEffect::draw(GLdouble deltaTime)
{
#ifdef PROFILE
GLuint queries[queriesSize];
GLuint queriesForRadixSort[queriesForRadixSortSize];
glGenQueries(queriesSize, queries);
glGenQueries(queriesForRadixSortSize, queriesForRadixSort);

auto tBegin = std::chrono::steady_clock::now();
#endif

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	GLfloat dt = ((GLfloat)deltaTime) * 50 * runtimeParams.TimeScale;
	//GLfloat dt = 0.01666666 * runtimeParams.TimeScale;

#ifdef VLAIDATE
Particle* prtcls = readFromBuffer<Particle>(currentParticlesCount, ssboParticles);
initValidation(prtcls);

GLuint* cells0 = nullptr;
GLuint* obj0 = nullptr;
GLuint* cells1 = nullptr;
GLuint* obj1 = nullptr;
GLuint* cells2 = nullptr;
GLuint* obj2 = nullptr;
GLuint* globalCounters = nullptr;
GLuint* totalSumms = nullptr;
#endif

	GLdouble oldCursorPosX = cursorPosX, oldCursorPosY = cursorPosY;
	glfwGetCursorPos(window, &cursorPosX, &cursorPosY);

	if (!isPaused || isAdvanceOneFrame)
	{
		for (GLuint subSteps = 0; subSteps < runtimeParams.substeps; subSteps++)
		{
#ifdef PROFILE
glBeginQuery(GL_TIME_ELAPSED, queries[0]);
#endif
			glUseProgram(fillCellIdAndObjectIdArraysCompShaderProgram);
			glUniform2f(0, windowWidth, windowHeight);
			glUniform1ui(1, runtimeParams.cellSize);
			GLuint groupCount = ceil(currentParticlesCount / 64.0f);
			glDispatchCompute(groupCount, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

#ifdef VLAIDATE
cells0 = readFromBuffer<GLuint>(currentCellsCount, ssboCellId);
obj0 = readFromBuffer<GLuint>(currentCellsCount, ssboObjectId);
validateFilledArrays(cells0, obj0);
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
#endif

			for(int i = 0; i < totalSortPasses; i++)
			{
#ifdef VLAIDATE
GLuint phase3CellsBuffer = buffer5;
GLuint phase3ObjsBuffer = buffer7;
GLuint* cells = cells1;
GLuint* objs = obj1;
if (i % 2 != 0)
{
	phase3CellsBuffer = ssboCellId;
	phase3ObjsBuffer = ssboObjectId;
	cells = cells2;
	objs = obj2;
}
#endif

#ifdef PROFILE
glBeginQuery(GL_TIME_ELAPSED, queriesForRadixSort[i * 3]);
#endif
				GLuint cellIdShift = i * bitsPerSortPass;

				// ============== PHASE 1 =====================
				glUseProgram(RadixSortPasses[i].Phase1);
				glUniform1ui(0, cellIdShift);
				glDispatchCompute(phase1GroupCount, 1, 1);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // TODO figure out what barrirers to use
				// ============================================
//GLfloat* test = readFromBuffer<GLfloat>(200, bufferTest);
#ifdef VLAIDATE
globalCounters = readFromBuffer<GLuint>(sharedCountersLength * phase1GroupCount, ssboGlobalCounters);
validateSortPhase1(i, globalCounters);
delete[] globalCounters;
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queriesForRadixSort[i * 3 + 1]);
#endif

				// ============== PHASE 2 =====================
				glUseProgram(RadixSortPasses[i].Phase2);
				glDispatchCompute(radixCountersLength, 1, 1);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				// ============================================
#ifdef VLAIDATE
globalCounters = readFromBuffer<GLuint>(sharedCountersLength * phase1GroupCount, ssboGlobalCounters);
totalSumms = readFromBuffer<GLuint>(radixCountersLength, buffer);
validateSortPhase2(globalCounters, totalSumms);
delete[] globalCounters;
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queriesForRadixSort[i * 3 + 2]);
#endif

				// ============== PHASE 3 =====================
				glUseProgram(RadixSortPasses[i].Phase3);
				glUniform1ui(0, cellIdShift);
				glDispatchCompute(phase1GroupCount, 1, 1);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				// ============================================

#ifdef VLAIDATE
cells = readFromBuffer<GLuint>(currentCellsCount, phase3CellsBuffer);
objs = readFromBuffer<GLuint>(currentCellsCount, phase3ObjsBuffer);
validateSortPhase3(i, cells, objs);
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
#endif
			}

#ifdef PROFILE
glBeginQuery(GL_TIME_ELAPSED, queries[1]);
#endif

			GLuint cellsPerThread = 10; // TODO calculate this
			GLuint threadsInBlock = threadsInThreadGroup;
			GLuint blocks = currentCellsCount / (cellsPerThread * threadsInBlock) + 1;
			glUseProgram(findCollisionCellsCompShaderProgram);

			glUniform2f(0, windowWidth, windowHeight);
			glUniform1f(1, runtimeParams.particleSize);
			glUniform1ui(2, runtimeParams.cellSize);
			glUniform1f(3, runtimeParams.ForceScale);

			glDispatchCompute(blocks, 1, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}
	}

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queries[2]);
#endif

	// =========== mouse drag handling ==================
	glUseProgram(resolveCollisionsCompShaderProgram); // TODO maybe make it really resolve collisions

	glUniform2f(0, windowWidth, windowHeight);
	glUniform1f(1, runtimeParams.cellSize);
	glUniform2f(2, (float)cursorPosX, (float)cursorPosY);
	glUniform1i(3, isLeftMouseBtnPressed);
	glUniform1ui(4, currentCellsCount);
	glUniform1f(5, runtimeParams.particleSize);
	glUniform1i(6, isLeftMouseBtnReleased);
	glUniform2f(7, (float)(cursorPosX - oldCursorPosX), (float)(cursorPosY - oldCursorPosY));

	GLuint groupsCOunt = ceil(currentCellsCount / 1024.0);
	glDispatchCompute(groupsCOunt, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	// ===================================================

	if (isDebug)
	{
		glUseProgram(gridShaderProgram);
		glUniform1f(0, runtimeParams.cellSize);
		glUniform2f(1, (float)cursorPosX, (float)cursorPosY);
		glUniform1i(2, windowHeight);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queries[3]);
#endif

	glUseProgram(shaderProgram);

	// vertex shader params
	glUniform2f(0, windowWidth, windowHeight);
	glUniform1f(1, runtimeParams.VelocityDamping);
	glUniform1f(2, dt);
	glUniform1i(3, isPaused && !isAdvanceOneFrame);
	glUniform1f(4, runtimeParams.particleSize);
	glUniform1ui(5, runtimeParams.cellSize);

	glPointSize(runtimeParams.particleSize);
	glDrawArrays(GL_POINTS, 0, currentParticlesCount);

#ifdef MISC_TESTS
GLuint* cells = readFromBuffer<GLuint>(currentCellsCount, buffer6);
GLuint* obj = readFromBuffer<GLuint>(currentCellsCount, buffer8);
GLfloat* test = readFromBuffer<GLfloat>(1000, bufferTest);
Particle* particles = readFromBuffer<Particle>(currentParticlesCount, ssboParticles);

double mx, my;
glfwGetCursorPos(window, &mx, &my);
int objIdUnderMouse = -1;
int collisions = 0;
for (int i = 0; i < currentParticlesCount; i++)
{
	if (isnan(particles[i].PosX))
	{
		__debugbreak();
	}

	//float deltaMX = particles[i].PosX - mx;
	//float deltaMY = particles[i].PosY - my;
	//float distSqured = deltaMX * deltaMX + deltaMY * deltaMY;
	//if (distSqured < (runtimeParams.particleSize / 2) * (runtimeParams.particleSize / 2))
	//{
	//	objIdUnderMouse = i;
	//}

	//for (int j = i + 1; j < currentParticlesCount; j++)
	//{
	//	float deltaX = particles[i].PosX - particles[j].PosX;
	//	float deltaY = particles[i].PosY - particles[j].PosY;
	//	float distSqured = deltaX * deltaX + deltaY * deltaY;
	//	if (distSqured < runtimeParams.particleSize * runtimeParams.particleSize)
	//		collisions++;
	//}
}
//std::cout << objIdUnderMouse << std::endl;
memcpy(particlesPrev, particles, currentParticlesCount * sizeof(Particle));

delete[] cells;
delete[] obj;
delete[] test;
delete[] particles;
#endif // MISC_TESTS

	isAdvanceOneFrame = false;
	isLeftMouseBtnPressed = false;
	isLeftMouseBtnReleased = false;
	frameCount++;

#ifdef VLAIDATE
clearValidation();
delete[] prtcls;
delete[] cells0;
delete[] obj0;
delete[] cells1;
delete[] obj1;
delete[] cells2;
delete[] obj2;
delete[] totalSumms;
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
auto tEnd = std::chrono::steady_clock::now();

currentlyAveragedFrames++;

GLint done = 0;
while (!done) { // waiting for GPU
	glGetQueryObjectiv(queries[queriesSize - 1], GL_QUERY_RESULT_AVAILABLE, &done);
}
done = 0;
while (!done) { // waiting for GPU
	glGetQueryObjectiv(queriesForRadixSort[queriesForRadixSortSize - 1], GL_QUERY_RESULT_AVAILABLE, &done);
}

GLuint64 elapsedTimes[queriesSize], elapsedTimesSort[queriesForRadixSortSize];
for (int i = 0; i < queriesSize; i++)
{
	glGetQueryObjectui64v(queries[i], GL_QUERY_RESULT, &elapsedTimes[i]);
}
for (int i = 0; i < queriesForRadixSortSize; i++)
{
	glGetQueryObjectui64v(queriesForRadixSort[i], GL_QUERY_RESULT, &elapsedTimesSort[i]);
}

float d = 1000000.0; // nanoseconds to miliseconds
accumulatedTimeCpu += std::chrono::duration_cast<std::chrono::nanoseconds>(tEnd - tBegin).count() / d;
averageFrameTimeCpu = accumulatedTimeCpu / (float)currentlyAveragedFrames;
for (int i = 0; i < queriesSize; i++)
{
	accumulatedTimeGpu[i] += elapsedTimes[i] / d;
	averageFrameTimeGpu[i] = accumulatedTimeGpu[i] / (float)currentlyAveragedFrames;
}
for (int i = 0; i < queriesForRadixSortSize; i++)
{
	accumulatedTimeGpuRadixSort[i] += elapsedTimesSort[i] / d;
	averageFrameTimeGpuRadixSort[i] = accumulatedTimeGpuRadixSort[i] / (float)currentlyAveragedFrames;
}

if (currentlyAveragedFrames % framesToAvegare == 0)
{
	currentlyAveragedFrames = 0;
	accumulatedTimeCpu = 0.0;
	for (int i = 0; i < queriesSize; i++)
	{
		accumulatedTimeGpu[i] = 0.0;
	}
	for (int i = 0; i < queriesForRadixSortSize; i++)
	{
		accumulatedTimeGpuRadixSort[i] = 0.0;
	}
}
#endif
}

void Particles2dCollisionEffect::drawGUI()
{
	ImGui::Begin("Startup params (Collisions)");
	ImGui::Text("Space: pause");
	ImGui::Text("While puased Arrow Right: advance one frame");
	ImGui::InputInt("Particles count", &startupParams.ParticlesCount, 1000, 10000);
	ImGui::Checkbox("N-body gravity", &startupParams.IsNBodyGravity);
	ImGui::End();

	ImGui::Begin("Runtime params (Collisions)");
	ImGui::SliderFloat("Force scale", &runtimeParams.ForceScale, 0.0, 2.0);
	ImGui::SliderFloat("Velocity damping", &runtimeParams.VelocityDamping, 0.9, 1.0);
	ImGui::SliderFloat("Time scale", &runtimeParams.TimeScale, 0.0, 5.0);
	ImGui::SliderFloat("Particle Size", &runtimeParams.particleSize, 1.0, 500.0);
	ImGui::SliderFloat("Cell Size", &runtimeParams.cellSize, 4.0, 500.0);
	ImGui::SliderInt("Substeps", &runtimeParams.substeps, 1.0, 128.0);
	ImGui::End();

	if (runtimeParams.cellSize < runtimeParams.particleSize)
	{
		runtimeParams.cellSize = runtimeParams.particleSize;
	}

#ifdef PROFILE
ImGui::Begin("Average frame time, ms");
ImGui::Text(("CPU - " + std::to_string(averageFrameTimeCpu)).c_str());

ImGui::Text(("Fill cellIds - "               + std::to_string(averageFrameTimeGpu[0])).c_str());
ImGui::Text(("Find collision cells - "       + std::to_string(averageFrameTimeGpu[1])).c_str());
ImGui::Text(("Handle mouse and draw grid - " + std::to_string(averageFrameTimeGpu[2])).c_str());
ImGui::Text(("Resolve collisions - "         + std::to_string(averageFrameTimeGpu[3])).c_str());
float totalSort = 0;
for (int i = 0; i < totalSortPasses; i++)
{
	ImGui::Text(("Sort phase 1 pass " + std::to_string(i) + " - " + std::to_string(averageFrameTimeGpuRadixSort[i * 3])).c_str());
	ImGui::Text(("Sort phase 2 pass " + std::to_string(i) + " - " + std::to_string(averageFrameTimeGpuRadixSort[i * 3 + 1])).c_str());
	ImGui::Text(("Sort phase 3 pass " + std::to_string(i) + " - " + std::to_string(averageFrameTimeGpuRadixSort[i * 3 + 2])).c_str());
	totalSort += averageFrameTimeGpuRadixSort[i * 3]
		+ averageFrameTimeGpuRadixSort[i * 3 + 1]
		+ averageFrameTimeGpuRadixSort[i * 3 + 2];
}
ImGui::Text(("Total sort - " + std::to_string(totalSort)).c_str());
ImGui::End();
#endif
}

void Particles2dCollisionEffect::restart()
{
	cleanup();
	initialize();
}

void Particles2dCollisionEffect::cleanup()
{
	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &ssboParticles);
	glDeleteVertexArrays(1, &vao);
}

void Particles2dCollisionEffect::keyCallback(int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		isPaused = !isPaused;
	}

	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS && isPaused)
	{
		isAdvanceOneFrame = true;
	}

	if (key == GLFW_KEY_D && action == GLFW_PRESS)
	{
		isDebug = !isDebug;
	}
}

void Particles2dCollisionEffect::mouseButtonCallback(int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		isLeftMouseBtnPressed = true;
		isLeftMouseBtnDown = true;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		isLeftMouseBtnReleased = true;
		isLeftMouseBtnDown = false;
	}
}

void Particles2dCollisionEffect::createComputeShaderProgram(GLuint& compShaderProgram, const char* shaderFilePath, std::vector<ShaderParam> shaderParams)
{
	GLint newShaderProgram = glCreateProgram();
	if (createShader(shaderFilePath, newShaderProgram, GL_COMPUTE_SHADER, shaderParams) == -1)
	{
		throw "Compute shader compilation failed";
	}
	glLinkProgram(newShaderProgram);
	if (checkShaderPrgramLinkErrors(newShaderProgram) == -1)
	{
		glDeleteProgram(newShaderProgram);
		throw "Compute shader program linking failed";
	}
	compShaderProgram = newShaderProgram;
}

template< typename T >
T* Particles2dCollisionEffect::readFromBuffer(int elemCount, GLuint ssbo)
{
	T* buf = new T[elemCount];
	T* p = (T*)glMapNamedBufferRange(ssbo, 0, elemCount * sizeof(T), GL_MAP_READ_BIT);
	memcpy(buf, p, elemCount * sizeof(T));
	glUnmapNamedBuffer(ssbo);

	return buf;
}


void Particles2dCollisionEffect::initValidation(Particle* particles)
{
	vParticles = particles;

	vGlobalCountersCount = phase1GroupCount * sharedCountersLength;

	vCellIds = std::vector<GLuint>(currentCellsCount);
	vObjectIds = std::vector<GLuint>(currentCellsCount);
	vCellIdsOutput = std::vector<GLuint>(currentCellsCount);
	vObjectIdsOutput = std::vector<GLuint>(currentCellsCount);
	vGlobalCounters = std::vector<GLuint>(vGlobalCountersCount);
	vTotalSumms = std::vector<GLuint>(radixCountersLength);
}

void Particles2dCollisionEffect::validateFilledArrays(GLuint* cellsFromGPU, GLuint* objectsFromGPU)
{
	GLuint gridWidth = windowWidth / runtimeParams.cellSize + 2;

	for (int i = 0; i < currentParticlesCount; i++)
	{
		Particle p = vParticles[i];

		double gridPosX, gridPosY;
		double cellPosX = modf(p.PosX / runtimeParams.cellSize, &gridPosX);
		double cellPosY = modf(p.PosY / runtimeParams.cellSize, &gridPosY);
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

		vCellIds[i] = hCellId;
		vCellIds[i + currentParticlesCount] = pCellIdsX;
		vCellIds[i + currentParticlesCount * 2] = pCellIdsY;
		vCellIds[i + currentParticlesCount * 3] = pCellIdsZ;

		vObjectIds[i] = i | 0x80000000;
		vObjectIds[i + currentParticlesCount] = i;
		vObjectIds[i + currentParticlesCount * 2] = i;
		vObjectIds[i + currentParticlesCount * 3] = i;
	}

	for (int i = 0; i < currentParticlesCount; i++)
	{
		if (cellsFromGPU[i] != vCellIds[i])
		{
			__debugbreak();
		}
		if (objectsFromGPU[i] != vObjectIds[i])
		{
			__debugbreak();
		}
	}
}

void Particles2dCollisionEffect::validateSortPhase1(GLuint pass, GLuint* globalCountersFromGPU)
{
	GLuint cellIdShift = pass * bitsPerSortPass;

	for (int workGroupID = 0; workGroupID < phase1GroupCount; workGroupID++)
	{
		GLuint globalCountersOffset = workGroupID * sharedCountersLength;

		for (int localInvocationID = 0; localInvocationID < threadsInWorkGroupInPhases1And3; localInvocationID++)
		{
			GLuint indexInGroup = localInvocationID % threadsInThreadGroup;
			GLuint groupIndex = localInvocationID / threadsInThreadGroup;
			GLuint cellIndexToReadFrom = (workGroupID * threadGroupsInWorkGroup + groupIndex) * elementsPerGroup + indexInGroup;
			GLuint counterIndexOffset = groupIndex * radixCountersLength;

			for (int i = 0; i < elementsPerGroup; i += threadsInThreadGroup)
			{
				if (cellIndexToReadFrom + i < currentCellsCount)
				{
					GLuint cellId = vCellIds[cellIndexToReadFrom + i];
					GLuint radix = (cellId >> cellIdShift) & bitMask;
					GLuint indexInSharedCounters = counterIndexOffset + radix;

					vGlobalCounters[indexInSharedCounters + globalCountersOffset]++;
				}
			}
		}
	}

	for (int i = 0; i < vGlobalCountersCount; i++)
	{
		if (globalCountersFromGPU[i] != vGlobalCounters[i])
		{
			__debugbreak();
			if (i == -1)
			{
				std::ofstream MyFile("c:/ValidateSortPhase1 globalCountersFromGPU.txt");
				for (int b = 0; b < vGlobalCountersCount; b += radixCountersLength)
				{
					for (int c = 0; c < radixCountersLength; c++)
					{
						if (globalCountersFromGPU[b + c] == 0)
						{
							MyFile << " " << "\t";
						}
						else
						{
							MyFile << globalCountersFromGPU[b + c] << "\t";
						}
					}
					MyFile << "\r";
				}
				MyFile.close();

				std::ofstream MyFile2("c:/ValidateSortPhase1 globalCounters.txt");
				for (int b = 0; b < vGlobalCountersCount; b += radixCountersLength)
				{
					for (int c = 0; c < radixCountersLength; c++)
					{
						if (vGlobalCounters[b + c] == 0)
						{
							MyFile2 << " " << "\t";
						}
						else
						{
							MyFile2 << vGlobalCounters[b + c] << "\t";
						}
					}
					MyFile2 << "\r";
				}
				MyFile2.close();
			}
		}
	}
}

void Particles2dCollisionEffect::validateSortPhase2(GLuint* globalCountersFromGPU, GLuint* totalSummsFromGPU)
{
	for (int radix = 0; radix < radixCountersLength; radix++)
	{
		GLuint summ = 0;
		for (int threadGroup = 0; threadGroup < threadGroupsTotal; threadGroup++)
		{
			GLuint globalCounterIndex = threadGroup * radixCountersLength + radix;

			GLuint counter = vGlobalCounters[globalCounterIndex];
			vGlobalCounters[globalCounterIndex] = summ;
			summ += counter;
		}
		vTotalSumms[radix] = summ;
	}

	for (int i = 0; i < vGlobalCountersCount; i++)
	{
		if (globalCountersFromGPU[i] != vGlobalCounters[i])
		{
			__debugbreak();
			if (i == -1)
			{
				std::ofstream MyFile("c:/ValidateSortPhase2 globalCountersFromGPU.txt");
				for (int b = 0; b < vGlobalCountersCount; b += radixCountersLength)
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
		if (totalSummsFromGPU[i] != vTotalSumms[i])
		{
			__debugbreak();
		}
	}
}

void Particles2dCollisionEffect::validateSortPhase3(GLuint pass, GLuint* cellsFromGPU, GLuint* objectsFromGPU)
{
	GLuint summ = 0;
	for (int i = 0; i < radixCountersLength; i++)
	{
		GLuint counter = vTotalSumms[i];
		vTotalSumms[i] = summ;
		summ += counter;
	}

	GLuint cellIdShift = pass * bitsPerSortPass;

	for (int workGroupID = 0; workGroupID < phase1GroupCount; workGroupID++)
	{
		std::vector<GLuint> sharedCounters = std::vector<GLuint>(sharedCountersLength);
		for (int i = 0; i < sharedCountersLength; i++)
		{
			sharedCounters[i] = vGlobalCounters[workGroupID * sharedCountersLength + i] + vTotalSumms[i % radixCountersLength];
		}

		for (int localInvocationID = 0; localInvocationID < threadsInWorkGroupInPhases1And3; localInvocationID++)
		{
			GLuint indexInGroup = localInvocationID % threadsInThreadGroup;
			GLuint groupIndex = localInvocationID / (float)threadsInThreadGroup;
			GLuint cellIndexToReadFrom = (workGroupID * threadGroupsInWorkGroup + groupIndex) * elementsPerGroup + indexInGroup;
			GLuint counterIndexOffset = groupIndex * radixCountersLength;

			for (int i = 0; i < elementsPerGroup; i += threadsInThreadGroup)
			{
				if (cellIndexToReadFrom + i < currentCellsCount)
				{
					GLuint cellId = vCellIds[cellIndexToReadFrom + i];
					GLuint objectId = vObjectIds[cellIndexToReadFrom + i];
					GLuint radix = (cellId >> cellIdShift) & bitMask;
					GLuint indexInSharedCounters = counterIndexOffset + radix;

					GLuint offset = sharedCounters[indexInSharedCounters];

					vCellIdsOutput[offset] = cellId;
					vObjectIdsOutput[offset] = objectId;

					sharedCounters[indexInSharedCounters]++;
				}
			}
		}

		sharedCounters.clear();
	}

	if (cellsFromGPU != nullptr && objectsFromGPU != nullptr)
	{
		for (int i = 0; i < currentCellsCount; i++)
		{
			if (cellsFromGPU[i] != vCellIdsOutput[i])
			{
				__debugbreak();
			}
			if (objectsFromGPU[i] != vObjectIdsOutput[i])
			{
				__debugbreak();
			}
		}
	}

	vCellIds.clear();
	vObjectIds.clear();
	vCellIds.shrink_to_fit();
	vObjectIds.shrink_to_fit();

	vCellIds = vCellIdsOutput;
	vObjectIds = vObjectIdsOutput;

	vCellIdsOutput.clear();
	vObjectIdsOutput.clear();
	vGlobalCounters.clear();

	vGlobalCounters = std::vector<GLuint>(vGlobalCountersCount);
	vCellIdsOutput = std::vector<GLuint>(currentCellsCount);
	vObjectIdsOutput = std::vector<GLuint>(currentCellsCount);
}

void Particles2dCollisionEffect::clearValidation()
{
	vCellIds.clear();
	vObjectIds.clear();
	vGlobalCounters.clear();
	vTotalSumms.clear();
	vCellIdsOutput.clear();
	vObjectIdsOutput.clear();

	vCellIds.shrink_to_fit();
	vObjectIds.shrink_to_fit();
	vGlobalCounters.shrink_to_fit();
	vTotalSumms.shrink_to_fit();
	vCellIdsOutput.shrink_to_fit();
	vObjectIdsOutput.shrink_to_fit();
}
