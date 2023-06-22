// implementation of this article
// https://developer.nvidia.com/gpugems/gpugems3/part-v-physics-simulation/chapter-32-broad-phase-collision-detection-cuda

//#define VLAIDATE
//#define MISC_TESTS
//#define PROFILE

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
	startupParams.ParticlesCount = 100000;
	startupParams.IsNBodyGravity = false;

	runtimeParams = {};
	runtimeParams.ForceScale = 1.0;
	runtimeParams.VelocityDamping = 0.999;
	runtimeParams.TimeScale = 1.000;
	runtimeParams.particleSize = 2.0;
	runtimeParams.cellSize = 4.0;//ceil(sqrt(windowWidth * windowHeight / pow(2, cellIdBits)));
	runtimeParams.substeps = 1;

	runtimeParams.viewZoom = 1.0;
	runtimeParams.viewPosX = 0.0;
	runtimeParams.viewPosY = 0.0;

	zoomSpeed = 0.1;
}

Particles2dCollisionEffect::~Particles2dCollisionEffect()
{
	cleanup();
}

void Particles2dCollisionEffect::initialize()
{
	currentParticlesCount = startupParams.ParticlesCount;
	currentCellsCount = startupParams.ParticlesCount;

	phase1GroupCount = ceil(currentCellsCount / (float)threadsInWorkGroupInPhases1And3 / elementsPerThread);
	elementsPerGroup = threadsInThreadGroup * elementsPerThread;
	threadGroupsTotal = ceil(currentCellsCount / (double)elementsPerGroup);
	phase2Iterations = ceil(threadGroupsTotal / maxThreadsInWorkGroup / 2);
	workGroupsCountInFindCollisionCellsPhase = ceil(currentCellsCount / ((float)maxThreadsInWorkGroup * cellsPerThread));
	//workGroupsCountInFindCollisionCellsPhase = 5;

	initParticles();
	initBuffers();

	particlesPos.clear();
	particlesPrevPos.clear();
	particlesAcc.clear();
	particlesPressure.clear();

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
		shaderParams.cellIdsLength,
		shaderParams.cellsPerThread,
	};
	std::vector<ShaderParam> ResolveCollisionsShaderParams
	{
		shaderParams.cellIdsLength,
		shaderParams.cellsPerThread,
	};
	std::vector<ShaderParam> CompactCollisionCellsShaderParams
	{
		shaderParams.cellIdsLength,
		shaderParams.cellsPerThread,
	};
	std::vector<ShaderParam> MouseInteractionsShaderParams
	{
	};

	createComputeShaderProgram(fillCellIdAndObjectIdArraysCompShaderProgram, "FillCellIdAndObjectIdArrays.comp", fragShaderParams);
	createComputeShaderProgram(findCollisionCellsCompShaderProgram, "FindCollisionCells.comp", findCollisionCellsShaderParams);
	createComputeShaderProgram(compactCollisionCellsCompShaderProgram, "CompactCollisionCells.comp", CompactCollisionCellsShaderParams);
	createComputeShaderProgram(resolveCollisionsCompShaderProgram, "ResolveCollisions.comp", ResolveCollisionsShaderParams);
	createComputeShaderProgram(mouseInteractionsCompShaderProgram, "MouseInteractions.comp", MouseInteractionsShaderParams);

	initRadixSortShaderProgramms(shaderParams);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPointSize(runtimeParams.particleSize * runtimeParams.viewZoom);

#ifdef MISC_TESTS
particlesPrev = new Particle[currentParticlesCount];
#endif
int work_grp_cnt[3];
glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
printf("max global (total) work group counts x:%i y:%i z:%i\n", work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);
int work_grp_size[3];
glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n", work_grp_size[0], work_grp_size[1], work_grp_size[2]);
int work_grp_inv;
glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv); // TODO use this!
printf("max local work group invocations %i\n", work_grp_inv);
}

void Particles2dCollisionEffect::initParticles()
{
	srand(50);

	particlesPos = std::vector<Vec2>(currentParticlesCount);
	particlesPrevPos = std::vector<Vec2>(currentParticlesCount);
	particlesAcc = std::vector<Vec2>(currentParticlesCount);
	particlesPressure = std::vector<GLfloat>(currentParticlesCount);

	/*int t = 0;
	for (int x = 0; x < 10; x++)
	for (int y = 0; y < 10; y++)
	{
		particles[t]  .PosX = 501 + x * 6;
		particles[t++].PosY = 501 + y * 6;
	}*/

	GLfloat halfParticle = runtimeParams.particleSize / 2;
	for (int i = 0; i < currentParticlesCount; i++)
	{
		GLfloat x = random(halfParticle, windowWidth - halfParticle);
		GLfloat y = random(halfParticle, windowHeight - halfParticle);
		particlesPos[i].X = x;
		particlesPos[i].Y = y;

		//particles[i].PosX = random(101.0, 1001.0);
		//particles[i].PosY = random(101.0, 702.0);

		//particles[i].PosX += 40;
		//particles[i].PosY += 40;

		particlesPrevPos[i].X = x;// +random(-10, 10);
		particlesPrevPos[i].Y = y;// +random(-10, 10);


		//particles[i].VelX = random(-3.0, 3.0);
		//particles[i].VelY = random(-3.0, 3.0);
	}
}

void Particles2dCollisionEffect::initBuffers()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLsizeiptr sUint = sizeof(GLuint);
	GLsizeiptr sVec2 = sizeof(Vec2);
	GLsizeiptr sFloat = sizeof(GLfloat);

	createSsbo(&ssboParticlesPos,      0,  currentParticlesCount * sVec2,        &particlesPos[0], GL_DYNAMIC_DRAW);
	createSsbo(&ssboCellId,            1,  currentCellsCount * sUint,                        NULL, GL_DYNAMIC_DRAW);
	createSsbo(&ssboObjectId,          2,  currentCellsCount * sUint,                        NULL, GL_DYNAMIC_DRAW);
	createSsbo(&ssboGlobalCounters,    3,  sharedCountersLength * phase1GroupCount * sUint,  NULL, GL_DYNAMIC_DRAW);
	createSsbo(&buffer,                4,  radixCountersLength * sUint,                      NULL, GL_DYNAMIC_DRAW);
	createSsbo(&buffer5,               5,  currentCellsCount * sUint,                        NULL, GL_DYNAMIC_DRAW);
	createSsbo(&buffer6,               6,  currentCellsCount * sUint,                        NULL, GL_DYNAMIC_DRAW);
	createSsbo(&buffer7,               7,  currentCellsCount * sUint,                        NULL, GL_DYNAMIC_DRAW);
	createSsbo(&buffer8,               8,  currentCellsCount * sUint,                        NULL, GL_DYNAMIC_DRAW);
	createSsbo(&bufferTest,            9,  20000 * sFloat,                                   NULL, GL_DYNAMIC_DRAW);
	createSsbo(&ssboCollisionList,     10, currentCellsCount * sUint,                        NULL, GL_DYNAMIC_DRAW);
	createSsbo(&ssboCellIdsLookup,     11, maxNumberOfCells * sUint,                         NULL, GL_DYNAMIC_DRAW);
	createSsbo(&ssboCollCellsFound,    12, workGroupsCountInFindCollisionCellsPhase * sUint, NULL, GL_DYNAMIC_DRAW);
	createSsbo(&ssboParticlesPrevPos,  13, currentParticlesCount * sVec2,    &particlesPrevPos[0], GL_DYNAMIC_DRAW);
	createSsbo(&ssboParticlesAcc,      14, currentParticlesCount * sVec2,        &particlesAcc[0], GL_DYNAMIC_DRAW);
	createSsbo(&ssboParticlesPressure, 15, currentParticlesCount * sFloat,  &particlesPressure[0], GL_DYNAMIC_DRAW);
	createSsbo(&ssboMisc,              16, sUint,                                            NULL, GL_DYNAMIC_DRAW);
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
	shaderParams.elementsPerThread =       { "#define elementsPerThread 0",        "#define elementsPerThread "       + std::to_string(elementsPerThread) };
	shaderParams.threadGroupsTotal =       { "#define threadGroupsTotal 1",        "#define threadGroupsTotal "       + std::to_string(threadGroupsTotal) };
	shaderParams.phase2Iterations =        { "#define iterations 0",               "#define iterations "              + std::to_string(phase2Iterations) };
	shaderParams.sharedCountersLength =    { "#define sharedCountersLength 1",     "#define sharedCountersLength "    + std::to_string(sharedCountersLength) };
	shaderParams.bitMask =                 { "#define bitMask 1",                  "#define bitMask "                 + std::to_string(bitMask) };
	shaderParams.cellsPerThread =          { "#define cellsPerThread 1",           "#define cellsPerThread "          + std::to_string(cellsPerThread) };

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
		shaderParams.elementsPerThread,
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
		shaderParams.elementsPerThread,
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
	glPointSize(runtimeParams.particleSize * runtimeParams.viewZoom);

	//GLfloat dt = ((GLfloat)deltaTime) * 50 * runtimeParams.TimeScale;
	GLfloat dt = 0.01666666 * 50 * runtimeParams.TimeScale;

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
	GLfloat cursorDX = cursorPosX - oldCursorPosX;
	GLfloat cursorDY = cursorPosY - oldCursorPosY;

	if (isRightMouseBtnDown)
	{
		runtimeParams.viewPosX += cursorDX / runtimeParams.viewZoom;
		runtimeParams.viewPosY += cursorDY / runtimeParams.viewZoom;
	}

	for (GLuint subSteps = 0; subSteps < runtimeParams.substeps; subSteps++)
	{
#ifdef PROFILE
glBeginQuery(GL_TIME_ELAPSED, queries[0]);
#endif
		// TODO test how slow this is, maybe move clearing to collision resolve phase
		glClearNamedBufferData(ssboCollisionList, GL_R32I, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		glClearNamedBufferData(ssboCellIdsLookup, GL_R32I, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);

		glUseProgram(fillCellIdAndObjectIdArraysCompShaderProgram);
		glUniform2f(0, simulationAreaWidth, simulationAreaHeight);
		glUniform1ui(1, runtimeParams.cellSize);
		GLuint groupCount = ceil(currentParticlesCount / 1024.0f);
		glDispatchCompute(groupCount, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
#ifdef VLAIDATE
cells0 = readFromBuffer<GLuint>(currentCellsCount, ssboCellId);
obj0 = readFromBuffer<GLuint>(currentCellsCount, ssboObjectId);
validateFilledArrays(cells0, obj0);
#endif

		if (!isPaused || isAdvanceOneFrame)
		{
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
//GLfloat* test = readFromBuffer<GLfloat>(1, bufferTest);
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
delete[] cells;
delete[] objs;
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
#endif
			}

#ifdef PROFILE
glBeginQuery(GL_TIME_ELAPSED, queries[1]);
#endif

			glUseProgram(findCollisionCellsCompShaderProgram);

			glUniform2f(0, simulationAreaWidth, simulationAreaHeight);
			glUniform1f(1, runtimeParams.particleSize);
			glUniform1ui(2, runtimeParams.cellSize);
			glUniform1f(3, runtimeParams.ForceScale);

			glDispatchCompute(workGroupsCountInFindCollisionCellsPhase, 1, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);


			GLuint* collCellsFoundPerWorkGroup = readFromBuffer<GLuint>(workGroupsCountInFindCollisionCellsPhase, ssboCollCellsFound);

			int totalCollisionCells = 0;
			for(int i = 0; i < workGroupsCountInFindCollisionCellsPhase; i++)
			{
				totalCollisionCells += collCellsFoundPerWorkGroup[i];
			}
			int workGroups = ceil(totalCollisionCells / maxThreadsInWorkGroup);

			glUseProgram(compactCollisionCellsCompShaderProgram);

			glUniform1ui(0, workGroupsCountInFindCollisionCellsPhase);
			glUniform1f(1, runtimeParams.particleSize);
			glUniform1ui(2, runtimeParams.cellSize);
			glUniform1f(3, runtimeParams.ForceScale);
			glUniform2f(4, simulationAreaWidth, simulationAreaHeight);

			glDispatchCompute(workGroups, 1, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
#ifdef VLAIDATE
GLuint* collis = readFromBuffer<GLuint>(currentCellsCount, ssboCollisionList);
GLuint* lookup = readFromBuffer<GLuint>(maxNumberOfCells, ssboCellIdsLookup);

validateFindCollisionCells(collis, collCellsFoundPerWorkGroup, lookup);

delete[] lookup;
delete[] collis;
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queries[2]);
#endif

			glUseProgram(resolveCollisionsCompShaderProgram);

			glUniform1ui(0, workGroupsCountInFindCollisionCellsPhase);
			glUniform1f(1, runtimeParams.particleSize);
			glUniform1ui(2, runtimeParams.cellSize);
			glUniform1f(3, runtimeParams.ForceScale);
			glUniform2f(4, simulationAreaWidth, simulationAreaHeight);
			glUniform1ui(5, totalCollisionCells);
			glUniform1ui(6, runtimeParams.substeps);

			glDispatchCompute(workGroups, 1, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
//test = readFromBuffer<GLfloat>(1024, bufferTest);
//GLuint* cells = readFromBuffer<GLuint>(currentCellsCount, ssboCellId);
//GLuint* objs = readFromBuffer<GLuint>(currentCellsCount, ssboObjectId);
//GLuint* collis = readFromBuffer<GLuint>(currentCellsCount, ssboCollisionList);
//GLuint* missss = readFromBuffer<GLuint>(1, ssboMisc);
//GLfloat* test = readFromBuffer<GLfloat>(1024, bufferTest);
//frameCount2++;
//if (frameCount2 == 42)__debugbreak();
//glClearNamedBufferData(bufferTest, GL_R32F, GL_RED, GL_FLOAT, NULL);
		}
	}

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queries[3]);
#endif

	// =========== mouse drag handling ==================
	glUseProgram(mouseInteractionsCompShaderProgram);

	glUniform2f(0, simulationAreaWidth, simulationAreaHeight);
	glUniform1f(1, runtimeParams.cellSize);
	glUniform2f(2, (float)cursorPosX, (float)cursorPosY);
	glUniform1i(3, isLeftMouseBtnPressed);
	glUniform1ui(4, currentCellsCount);
	glUniform1f(5, runtimeParams.particleSize);
	glUniform1i(6, isLeftMouseBtnReleased);
	glUniform2f(7, cursorDX, cursorDY);
	glUniform2f(8, runtimeParams.viewPosX, runtimeParams.viewPosY);
	glUniform1f(9, runtimeParams.viewZoom);

	GLuint groupsCount = ceil(currentCellsCount / 1024.0);
	glDispatchCompute(groupsCount, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	// ===================================================

	if (isDebug)
	{
		glUseProgram(gridShaderProgram);
		glUniform1f(0, runtimeParams.cellSize);
		glUniform2f(1, (float)cursorPosX, (float)cursorPosY);
		glUniform2f(2, simulationAreaWidth, simulationAreaHeight);
		glUniform2f(3, runtimeParams.viewPosX, runtimeParams.viewPosY);
		glUniform1f(4, runtimeParams.viewZoom);
		glUniform2f(5, windowWidth, windowHeight);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queries[4]);
#endif

	glUseProgram(shaderProgram);

	// vertex shader params
	glUniform2f(0, windowWidth, windowHeight);
	glUniform1f(1, runtimeParams.VelocityDamping);
	glUniform1f(2, dt);
	glUniform1i(3, isPaused && !isAdvanceOneFrame);
	glUniform1f(4, runtimeParams.particleSize);
	glUniform1ui(5, runtimeParams.cellSize);
	glUniform2f(6, runtimeParams.viewPosX, runtimeParams.viewPosY);
	glUniform1f(7, runtimeParams.viewZoom);
	glUniform2f(8, simulationAreaWidth, simulationAreaHeight);

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
	ImGui::Text("Perticle drag: left mouse button");
	ImGui::Text("Pause: space");
	ImGui::Text("While paused Arrow Right: advance one frame");
	ImGui::Text("Pan: right mouse button");
	ImGui::Text("Zoom: scroll");
	ImGui::Text("Fast zoom: left shift + scroll");
	ImGui::Text("Reset zoom and pan: middle mouse button");
	ImGui::InputInt("Particles count", &startupParams.ParticlesCount, 1000, 10000);
	ImGui::Checkbox("N-body gravity", &startupParams.IsNBodyGravity);
	ImGui::End();

	ImGui::Begin("Runtime params (Collisions)");
	ImGui::SliderFloat("Force scale", &runtimeParams.ForceScale, 0.0, 2.0);
	ImGui::SliderFloat("Velocity damping", &runtimeParams.VelocityDamping, 0.9, 1.0);
	ImGui::SliderFloat("Time scale", &runtimeParams.TimeScale, 0.0, 5.0);
	ImGui::SliderFloat("Particle Size", &runtimeParams.particleSize, 1.0, 50.0);
	ImGui::SliderFloat("Cell Size", &runtimeParams.cellSize, 4.0, 500.0);
	ImGui::SliderInt("Substeps", &runtimeParams.substeps, 1.0, 32.0);
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
ImGui::Text(("Resolve collisions - "         + std::to_string(averageFrameTimeGpu[2])).c_str());
ImGui::Text(("Handle mouse and draw grid - " + std::to_string(averageFrameTimeGpu[3])).c_str());
ImGui::Text(("Integration - "                + std::to_string(averageFrameTimeGpu[4])).c_str());
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
	glDeleteProgram(fillCellIdAndObjectIdArraysCompShaderProgram);
	glDeleteProgram(findCollisionCellsCompShaderProgram);
	glDeleteProgram(compactCollisionCellsCompShaderProgram);
	glDeleteProgram(resolveCollisionsCompShaderProgram);
	glDeleteProgram(mouseInteractionsCompShaderProgram);
	glDeleteProgram(gridShaderProgram);
	for (int i = 0; i < totalSortPasses; i++)
	{
		glDeleteProgram(RadixSortPasses[i].Phase1);
		glDeleteProgram(RadixSortPasses[i].Phase2);
		glDeleteProgram(RadixSortPasses[i].Phase3);
	}
	
	glClearNamedBufferData(ssboParticlesPos,      GL_R32F, GL_RED, GL_FLOAT, NULL);
	glClearNamedBufferData(ssboParticlesPrevPos,  GL_R32F, GL_RED, GL_FLOAT, NULL);
	glClearNamedBufferData(ssboParticlesAcc,      GL_R32F, GL_RED, GL_FLOAT, NULL);
	glClearNamedBufferData(ssboParticlesPressure, GL_R32F, GL_RED, GL_FLOAT, NULL);
	glClearNamedBufferData(ssboObjectId,          GL_R32I, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glClearNamedBufferData(ssboCellId,            GL_R32I, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glClearNamedBufferData(ssboGlobalCounters,    GL_R32I, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glClearNamedBufferData(ssboCollisionList,     GL_R32I, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glClearNamedBufferData(ssboMisc,              GL_R32I, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glClearNamedBufferData(ssboCollCellsFound,    GL_R32I, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glClearNamedBufferData(ssboCellIdsLookup,     GL_R32I, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);

	glDeleteBuffers(1, &ssboParticlesPos);
	glDeleteBuffers(1, &ssboParticlesPrevPos);
	glDeleteBuffers(1, &ssboParticlesAcc);
	glDeleteBuffers(1, &ssboParticlesPressure);
	glDeleteBuffers(1, &ssboObjectId);
	glDeleteBuffers(1, &ssboCellId);
	glDeleteBuffers(1, &ssboGlobalCounters);
	glDeleteBuffers(1, &ssboCollisionList);
	glDeleteBuffers(1, &ssboMisc);
	glDeleteBuffers(1, &ssboCollCellsFound);
	glDeleteBuffers(1, &ssboCellIdsLookup);

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

	if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS)
	{
		zoomSpeed = 1;
	}
	if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE)
	{
		zoomSpeed = 0.1;
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
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		isRightMouseBtnDown = true;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
	{
		isRightMouseBtnDown = false;
	}
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
	{
		resetPanAndZoom();
	}
}

void Particles2dCollisionEffect::scrollCallback(double xoffset, double yoffset)
{
	GLdouble beforeZoomCursorPosX = cursorPosX / runtimeParams.viewZoom + runtimeParams.viewPosX;
	GLdouble beforeZoomCursorPosY = cursorPosY / runtimeParams.viewZoom + runtimeParams.viewPosY;

	GLdouble multiplier = 1 + zoomSpeed;
	if (yoffset < 0)
	{
		runtimeParams.viewZoom /= multiplier;
	}
	else
	{
		runtimeParams.viewZoom *= multiplier;
	}

	GLdouble afterZoomCursorPosX = cursorPosX / runtimeParams.viewZoom + runtimeParams.viewPosX;
	GLdouble afterZoomCursorPosY = cursorPosY / runtimeParams.viewZoom + runtimeParams.viewPosY;

	runtimeParams.viewPosX -= beforeZoomCursorPosX - afterZoomCursorPosX;
	runtimeParams.viewPosY -= beforeZoomCursorPosY - afterZoomCursorPosY;
}

void Particles2dCollisionEffect::resetPanAndZoom()
{
	runtimeParams.viewZoom = 1.0;
	runtimeParams.viewPosX = 0.0;
	runtimeParams.viewPosY = 0.0;
	glPointSize(runtimeParams.particleSize);
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

void Particles2dCollisionEffect::createSsbo(GLuint* buff, GLuint index, GLsizeiptr size, const void* data, GLenum usage)
{
	glGenBuffers(1, buff);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, *buff);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usage);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

template< typename T >
T* Particles2dCollisionEffect::readFromBuffer(int elemCount, GLuint ssbo)
{
	T* buf = new T[elemCount];
	T* p = (T*)glMapNamedBufferRange(ssbo, 0, elemCount * sizeof(T), GL_MAP_READ_BIT);
	memcpy(buf, p, elemCount * sizeof(T));
	bool b = glUnmapNamedBuffer(ssbo);
	if(!b)__debugbreak();
	return buf;
}
template< typename T >
T* Particles2dCollisionEffect::readFromBuffer2(int elemCount, GLuint ssbo)
{
	T* buf = new T[elemCount];
	T* p = (T*)glMapNamedBufferRange(ssbo, 0, elemCount * sizeof(T), GL_MAP_READ_BIT);
	memcpy(buf, p, elemCount * sizeof(T));
	bool b = glUnmapNamedBuffer(ssbo);
	if (!b)__debugbreak();

	return buf;
}


void Particles2dCollisionEffect::initValidation(Vec2* particles)
{
	vParticles = particles;

	vGlobalCountersCount = phase1GroupCount * sharedCountersLength;

	vCellIds = std::vector<GLuint>(currentCellsCount);
	vObjectIds = std::vector<GLuint>(currentCellsCount);
	vCellIdsOutput = std::vector<GLuint>(currentCellsCount);
	vObjectIdsOutput = std::vector<GLuint>(currentCellsCount);
	vGlobalCounters = std::vector<GLuint>(vGlobalCountersCount);
	vTotalSumms = std::vector<GLuint>(radixCountersLength);
	vCollisionCells = std::vector<GLuint>();
}

void Particles2dCollisionEffect::validateFilledArrays(GLuint* cellsFromGPU, GLuint* objectsFromGPU)
{
	GLuint gridWidth = simulationAreaWidth / runtimeParams.cellSize;

	for (int i = 0; i < currentParticlesCount; i++)
	{
		Vec2 p = vParticles[i];

		double gridPosX, gridPosY;
		modf(p.X / runtimeParams.cellSize, &gridPosX);
		modf(p.Y / runtimeParams.cellSize, &gridPosY);

		GLuint hCellId = gridPosX + gridPosY * gridWidth;

		vCellIds[i] = hCellId;
		vObjectIds[i] = i;
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
			GLuint groupIndex = localInvocationID / (float)threadsInThreadGroup;
			GLuint counterIndexOffset = groupIndex * radixCountersLength;
			GLuint cellIndexToReadFrom = (workGroupID * threadGroupsInWorkGroup + groupIndex) * elementsPerGroup + indexInGroup;

			for (int i = 0; i < elementsPerGroup && cellIndexToReadFrom + i < currentCellsCount; i += threadsInThreadGroup)
			{
				GLuint cellId = vCellIds[cellIndexToReadFrom + i];
				GLuint radix = (cellId >> cellIdShift) & bitMask;
				GLuint indexInSharedCounters = counterIndexOffset + radix;

				vGlobalCounters[indexInSharedCounters + globalCountersOffset]++;
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

		for (int i = 0; i < elementsPerGroup; i += threadsInThreadGroup)
		for (int localInvocationID = 0; localInvocationID < threadsInWorkGroupInPhases1And3; localInvocationID++)
		{
			GLuint indexInGroup = localInvocationID % threadsInThreadGroup;
			GLuint groupIndex = localInvocationID / (float)threadsInThreadGroup;
			GLuint counterIndexOffset = groupIndex * radixCountersLength;
			GLuint cellIndexToReadFrom = (workGroupID * threadGroupsInWorkGroup + groupIndex) * elementsPerGroup + indexInGroup;

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

void Particles2dCollisionEffect::validateFindCollisionCells(GLuint* collsionCellsFromGPU, GLuint* collCellsFoundPerWorkGroupFromGPU, GLuint* lookupFromGPU)
{
	int threadsInWorkGroup = 4;

	std::vector<GLuint> collCellsFoundPerWorkGroup = std::vector<GLuint>();
	std::vector<GLuint> cellIdToCollisionListIdLookup = std::vector<GLuint>(maxNumberOfCells);

	int prevCell = -1;
	int totalCollisionsCells = 0;
	for (int workGroupID = 0; workGroupID < workGroupsCountInFindCollisionCellsPhase; workGroupID++)
	{
		int workGroupOffset = workGroupID * threadsInWorkGroup * cellsPerThread;
		int collisionsFound = 0;

		for (int localInvocationID = 0; localInvocationID < threadsInWorkGroup; localInvocationID++)
		{
			int totalOffset = workGroupOffset + localInvocationID * cellsPerThread;

			for (int i = 0; i < cellsPerThread && totalOffset + i < currentCellsCount; i++)
			{
				int currentCell = vCellIds[totalOffset + i];
				if (currentCell != prevCell)
				{
					vCollisionCells.push_back(totalOffset + i);
					cellIdToCollisionListIdLookup[currentCell] = vCollisionCells.size();
					collisionsFound++;
				}

				prevCell = currentCell;
			}
		}

		collCellsFoundPerWorkGroup.push_back(collisionsFound);
		totalCollisionsCells += collisionsFound;
	}

	if (collsionCellsFromGPU != nullptr && collCellsFoundPerWorkGroupFromGPU != nullptr && lookupFromGPU != nullptr)
	{
		for (int i = 0; i < totalCollisionsCells; i++)
		{
			if (collsionCellsFromGPU[i] != vCollisionCells[i])
			{
				__debugbreak();
			}
		}
		for (int i = 0; i < maxNumberOfCells; i++)
		{
			if (lookupFromGPU[i] != cellIdToCollisionListIdLookup[i])
			{
				__debugbreak();
			}
		}
		for (int i = 0; i < workGroupsCountInFindCollisionCellsPhase; i++)
		{
			if (collCellsFoundPerWorkGroupFromGPU[i] != collCellsFoundPerWorkGroup[i])
			{
				__debugbreak();
			}
		}
	}
}

void Particles2dCollisionEffect::clearValidation()
{
	vCellIds.clear();
	vObjectIds.clear();
	vGlobalCounters.clear();
	vTotalSumms.clear();
	vCellIdsOutput.clear();
	vObjectIdsOutput.clear();
	vCollisionCells.clear();

	vCellIds.shrink_to_fit();
	vObjectIds.shrink_to_fit();
	vGlobalCounters.shrink_to_fit();
	vTotalSumms.shrink_to_fit();
	vCellIdsOutput.shrink_to_fit();
	vObjectIdsOutput.shrink_to_fit();
	vCollisionCells.shrink_to_fit();
}
