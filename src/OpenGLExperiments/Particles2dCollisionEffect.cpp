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
	startupParams.ParticlesCount = 1000000;
	startupParams.IsNBodyGravity = false;

	runtimeParams = {};
	runtimeParams.ForceScale = 1.0;
	runtimeParams.VelocityDamping = 0.999;
	runtimeParams.TimeScale = 0.0001;
	runtimeParams.particleSize = 4.0;
	runtimeParams.cellSize = 4.0;//ceil(sqrt(windowWidth * windowHeight / 65535));
	runtimeParams.substeps = 1;
}

Particles2dCollisionEffect::~Particles2dCollisionEffect()
{
	cleanup();
}


void Particles2dCollisionEffect::initialize()
{
	srand(50);
	
	currentParticlesCount = startupParams.ParticlesCount;
	currentCellsCount = startupParams.ParticlesCount * 4;

	particles = std::vector<Particle>(currentParticlesCount);
	for (int i = 0; i < currentParticlesCount; i++)
	{
		particles[i].PosX = random(20.0, 1260.0);
		particles[i].PosY = random(20.0, 780.0);

		particles[i].PosXprev = particles[i].PosX;
		particles[i].PosYprev = particles[i].PosY;


		//particles[i].VelX = random(-3.0, 3.0);
		//particles[i].VelY = random(-3.0, 3.0);
	}


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


	glGenVertexArrays(1, &vaoCellId);
	glBindVertexArray(vaoCellId);

	glGenBuffers(1, &ssboCellId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCellId);
	glBufferData(GL_SHADER_STORAGE_BUFFER, currentCellsCount * sizeof(GLuint), NULL, GL_DYNAMIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	glGenVertexArrays(1, &vaoObjectId);
	glBindVertexArray(vaoObjectId);

	glGenBuffers(1, &ssboObjectId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboObjectId);
	glBufferData(GL_SHADER_STORAGE_BUFFER, currentCellsCount * sizeof(GLuint), NULL, GL_DYNAMIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboObjectId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	radixCountersLength = 256;
	sharedCountersLength = 12032; // 12288 is maximum on my laptop's 3060, but in phase 3 need some additional shared memory for total summs counting
	threadGroupsInWorkGroup = sharedCountersLength / radixCountersLength;
	threadsInThreadGroup = 16;
	threadsInWorkGroup = threadGroupsInWorkGroup * threadsInThreadGroup;
	phase1GroupCount = ceil(currentCellsCount / threadsInWorkGroup);// / 16;


	glGenVertexArrays(1, &vaoGlobalCounters);
	glBindVertexArray(vaoGlobalCounters);

	glGenBuffers(1, &ssboGlobalCounters);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboGlobalCounters);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sharedCountersLength * phase1GroupCount * sizeof(GLuint), NULL, GL_DYNAMIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboGlobalCounters);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	particles.clear();

	std::vector<ShaderParams> vertShaderParams
	{
		ShaderParams {"#define particlesCount 0", "#define particlesCount " + std::to_string(currentParticlesCount)},
		ShaderParams {"#define cellsCount 0",     "#define cellsCount "     + std::to_string(currentCellsCount)},
		ShaderParams { "#define nBody 0",         "#define nBody "          + std::to_string(startupParams.IsNBodyGravity ? 1 : 0) }
	};
	std::vector<ShaderParams> fragShaderParams
	{
		ShaderParams {"#define particlesCount 0", "#define particlesCount " + std::to_string(currentParticlesCount)}
	};
	GLint newShaderProgram = createShaderProgramFromFiles(vertShaderParams, fragShaderParams);
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}
	shaderProgram = newShaderProgram;

	newShaderProgram = createShaderProgramFromFiles("Grid.vert", "Grid.frag", std::vector<ShaderParams>(), std::vector<ShaderParams>());
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}
	gridShaderProgram = newShaderProgram;

	elementsPerThread = ceil(currentCellsCount / (phase1GroupCount * threadsInWorkGroup));
	elementsPerGroup = threadsInThreadGroup * elementsPerThread;
	threadGroupsTotal = ceil(currentCellsCount / (double)elementsPerGroup);
	GLuint phase2Iterations = ceil(threadGroupsTotal / 1024.0); // 1024 is maximum threads per work group on my laptop's 3060

	ShaderParams cellIdsLengthShaderParam          { "#define cellIdsLength 0",            "#define cellIdsLength "           + std::to_string(currentCellsCount) };
	ShaderParams threadsInWorkGroupShaderParam     { "#define threadsInWorkGroup 1",       "#define threadsInWorkGroup "      + std::to_string((int)threadsInWorkGroup) };
	ShaderParams threadGroupsInWorkGroupShaderParam{ "#define threadGroupsInWorkGroup 1",  "#define threadGroupsInWorkGroup " + std::to_string(threadGroupsInWorkGroup) };
	ShaderParams radixCountersLengthShaderParam    { "#define radixCountersLength 1",      "#define radixCountersLength "     + std::to_string(radixCountersLength) };
	ShaderParams threadsInThreadGroupShaderParam   { "#define threadsInThreadGroup 0",     "#define threadsInThreadGroup "    + std::to_string(threadsInThreadGroup) };
	ShaderParams elementsPerGroupShaderParam       { "#define elementsPerGroup 0",         "#define elementsPerGroup "        + std::to_string(elementsPerGroup) };
	ShaderParams threadGroupsTotalShaderParam      { "#define threadGroupsTotal 1",        "#define threadGroupsTotal "       + std::to_string(threadGroupsTotal) };
	ShaderParams phase2IterationsShaderParam       { "#define iterations 0",               "#define iterations "              + std::to_string(phase2Iterations) };
	ShaderParams bindingCellIds1ShaderParam        { "#define bindingCellIds1 1",          "#define bindingCellIds1 5"};
	ShaderParams bindingCellIds2ShaderParam        { "#define bindingCellIds2 5",          "#define bindingCellIds2 1"};
	ShaderParams cellIdShiftShaderParam            { "#define cellIdShift 0",              "#define cellIdShift 8"};
	ShaderParams cells1ShaderParam                 { "#define cellsFirst cells1",          "#define cellsFirst cells2"};
	ShaderParams cells2ShaderParam                 { "#define cellsSecond cells2",         "#define cellsSecond cells1"};

	ShaderParams bindingCellIdsInputShaderParam    { "#define bindingCellIdsInput 1",      "#define bindingCellIdsInput 5"};
	ShaderParams bindingCellIdsOutputShaderParam   { "#define bindingCellIdsOutput 5",     "#define bindingCellIdsOutput 6"};
	ShaderParams bufferCellIdsInputShaderParam     { "#define bufferCellIdsInput cells1",  "#define bufferCellIdsInput cells2"};
	ShaderParams bufferCellIdsOutputShaderParam    { "#define bufferCellIdsOutput cells2", "#define bufferCellIdsOutput cells3"};

	ShaderParams bindingObjectIdsInputShaderParam    { "#define bindingObjectIdsInput 2",        "#define bindingObjectIdsInput 7"};
	ShaderParams bindingObjectIdsOutputShaderParam   { "#define bindingObjectIdsOutput 7",       "#define bindingObjectIdsOutput 8"};
	ShaderParams bufferObjectIdsInputShaderParam     { "#define bufferObjectIdsInput objects1",  "#define bufferObjectIdsInput objects2"};
	ShaderParams bufferObjectIdsOutputShaderParam    { "#define bufferObjectIdsOutput objects2", "#define bufferObjectIdsOutput objects3"};

	std::vector<ShaderParams> phase1Pass1ShaderParams
	{
		cellIdsLengthShaderParam,
		radixCountersLengthShaderParam,
		threadsInWorkGroupShaderParam,
		threadGroupsInWorkGroupShaderParam,
		threadsInThreadGroupShaderParam,
		elementsPerGroupShaderParam
	};
	std::vector<ShaderParams> phase1Pass2ShaderParams
	{
		cellIdsLengthShaderParam,
		radixCountersLengthShaderParam,
		threadsInWorkGroupShaderParam,
		threadGroupsInWorkGroupShaderParam,
		threadsInThreadGroupShaderParam,
		elementsPerGroupShaderParam,

		cellIdShiftShaderParam,
		
		bindingCellIds1ShaderParam,
		bindingCellIds2ShaderParam,
		cells1ShaderParam,
		cells2ShaderParam
	};
	std::vector<ShaderParams> phase2ShaderParams
	{
		threadGroupsInWorkGroupShaderParam,
		radixCountersLengthShaderParam,
		threadsInThreadGroupShaderParam,
		elementsPerGroupShaderParam,
		threadGroupsTotalShaderParam,
		phase2IterationsShaderParam
	};
	std::vector<ShaderParams> phase3Pass1ShaderParams
	{
		cellIdsLengthShaderParam,
		threadsInWorkGroupShaderParam,
		threadGroupsInWorkGroupShaderParam,
		radixCountersLengthShaderParam,
		threadsInThreadGroupShaderParam,
		elementsPerGroupShaderParam
	};
	std::vector<ShaderParams> phase3Pass2ShaderParams
	{
		cellIdsLengthShaderParam,
		threadsInWorkGroupShaderParam,
		threadGroupsInWorkGroupShaderParam,
		radixCountersLengthShaderParam,
		threadsInThreadGroupShaderParam,
		elementsPerGroupShaderParam,

		cellIdShiftShaderParam,

		bindingCellIdsInputShaderParam,
		bindingCellIdsOutputShaderParam,
		bufferCellIdsInputShaderParam,
		bufferCellIdsOutputShaderParam,

		bindingObjectIdsInputShaderParam,
		bindingObjectIdsOutputShaderParam,
		bufferObjectIdsInputShaderParam,
		bufferObjectIdsOutputShaderParam
	};
	std::vector<ShaderParams> findCollisionCellsShaderParams
	{
		cellIdsLengthShaderParam
	};
	std::vector<ShaderParams> ResolveCollisionsShaderParams
	{
	};

	createComputeShaderProgram(fillCellIdAndObjectIdArraysCompShaderProgram, "FillCellIdAndObjectIdArrays.comp", fragShaderParams);
	createComputeShaderProgram(radixPhase1Pass1CompShaderProgram, "RadixSortPhase1.comp", phase1Pass1ShaderParams);
	createComputeShaderProgram(radixPhase1Pass2CompShaderProgram, "RadixSortPhase1.comp", phase1Pass2ShaderParams);
	createComputeShaderProgram(radixPhase2CompShaderProgram, "RadixSortPhase2.comp", phase2ShaderParams);
	createComputeShaderProgram(radixPhase3Pass1CompShaderProgram, "RadixSortPhase3.comp", phase3Pass1ShaderParams);
	createComputeShaderProgram(radixPhase3Pass2CompShaderProgram, "RadixSortPhase3.comp", phase3Pass2ShaderParams);
	createComputeShaderProgram(findCollisionCellsCompShaderProgram, "FindCollisionCells.comp", findCollisionCellsShaderParams);
	createComputeShaderProgram(resolveCollisionsCompShaderProgram, "ResolveCollisions.comp", ResolveCollisionsShaderParams);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

#ifdef VLAIDATE
validator = new Validator;
#endif
#ifdef MISC_TESTS
particlesPrev = new Particle[currentParticlesCount];
#endif
}

void Particles2dCollisionEffect::draw(GLdouble deltaTime)
{
#ifdef PROFILE
GLuint queries[10];
glGenQueries(10, queries);

auto tBegin = std::chrono::steady_clock::now();
#endif

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	//deltaTime = 0.01666666 * 2;
	GLfloat dt = ((GLfloat)deltaTime) * 50 * runtimeParams.TimeScale;

#ifdef VLAIDATE
Particle* prtcls = readFromBuffer<Particle>(currentParticlesCount, ssboParticles);
validator->Init(prtcls, currentParticlesCount, runtimeParams.particleSize, windowWidth, windowHeight, phase1GroupCount);

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
validator->ValidateFilledArrays(cells0, obj0);
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queries[1]);
#endif

			// ============== PHASE 1 PASS 1 =====================
			glUseProgram(radixPhase1Pass1CompShaderProgram);
			glDispatchCompute(phase1GroupCount, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			// ===================================================
#ifdef VLAIDATE
globalCounters = readFromBuffer<GLuint>(sharedCountersLength * phase1GroupCount, ssboGlobalCounters);
validator->ValidateSortPhase1(0, threadsInWorkGroup, threadGroupsInWorkGroup, threadsInThreadGroup, elementsPerGroup, globalCounters);
delete[] globalCounters;
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queries[2]);
#endif

			// ============== PHASE 2 PASS 1 =====================
			glUseProgram(radixPhase2CompShaderProgram);
			glDispatchCompute(radixCountersLength, 1, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			// ===================================================
#ifdef VLAIDATE
globalCounters = readFromBuffer<GLuint>(sharedCountersLength * phase1GroupCount, ssboGlobalCounters);
totalSumms = readFromBuffer<GLuint>(radixCountersLength, buffer);
validator->ValidateSortPhase2(threadGroupsTotal, globalCounters, totalSumms);
delete[] globalCounters;
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queries[3]);
#endif

			// ============== PHASE 3 PASS 1 =====================
			glUseProgram(radixPhase3Pass1CompShaderProgram);
			glDispatchCompute(phase1GroupCount, 1, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			// ===================================================
#ifdef VLAIDATE
cells1 = readFromBuffer<GLuint>(currentCellsCount, buffer5);
obj1 = readFromBuffer<GLuint>(currentCellsCount, buffer7);
validator->ValidateSortPhase3(0, threadsInWorkGroup, threadGroupsInWorkGroup, threadsInThreadGroup, elementsPerGroup, cells1, obj1);
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queries[4]);
#endif

			// ============== PHASE 1 PASS 2 =====================
			glUseProgram(radixPhase1Pass2CompShaderProgram);
			glDispatchCompute(phase1GroupCount, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			// ===================================================
#ifdef VLAIDATE
globalCounters = readFromBuffer<GLuint>(sharedCountersLength * phase1GroupCount, ssboGlobalCounters);
validator->ValidateSortPhase1(1, threadsInWorkGroup, threadGroupsInWorkGroup, threadsInThreadGroup, elementsPerGroup, globalCounters);
delete[] globalCounters;
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queries[5]);
#endif

			// ============== PHASE 2 PASS 2 =====================
			glUseProgram(radixPhase2CompShaderProgram);
			glDispatchCompute(radixCountersLength, 1, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			// ===================================================
#ifdef VLAIDATE
globalCounters = readFromBuffer<GLuint>(sharedCountersLength * phase1GroupCount, ssboGlobalCounters);
totalSumms = readFromBuffer<GLuint>(radixCountersLength, buffer);
validator->ValidateSortPhase2(threadGroupsTotal, globalCounters, totalSumms);
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queries[6]);
#endif

			// ============== PHASE 3 PASS 2 =====================
			glUseProgram(radixPhase3Pass2CompShaderProgram);
			glDispatchCompute(phase1GroupCount + 1, 1, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			// ===================================================
#ifdef VLAIDATE
cells2 = readFromBuffer<GLuint>(currentCellsCount, buffer6);
obj2 = readFromBuffer<GLuint>(currentCellsCount, buffer8);
validator->ValidateSortPhase3(1, threadsInWorkGroup, threadGroupsInWorkGroup, threadsInThreadGroup, elementsPerGroup, cells2, obj2);
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queries[7]);
#endif

			GLuint cellsPerThread = 10; // TODO calculate this
			GLuint threadsInBlock = 1024;
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
glBeginQuery(GL_TIME_ELAPSED, queries[8]);
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
//GLfloat* test = readFromBuffer<GLfloat>(200, bufferTest);

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
glBeginQuery(GL_TIME_ELAPSED, queries[9]);
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
validator->Clear();
delete[] prtcls;
delete[] cells0;
delete[] obj0;
delete[] cells1;
delete[] obj1;
delete[] cells2;
delete[] obj2;
delete[] globalCounters;
delete[] totalSumms;
#endif

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
auto tEnd = std::chrono::steady_clock::now();

GLint done = 0;
while (!done) {
	glGetQueryObjectiv(queries[9], GL_QUERY_RESULT_AVAILABLE, &done);
}

GLuint64 elapsedTimes[10];
glGetQueryObjectui64v(queries[0], GL_QUERY_RESULT, &elapsedTimes[0]);
glGetQueryObjectui64v(queries[1], GL_QUERY_RESULT, &elapsedTimes[1]);
glGetQueryObjectui64v(queries[2], GL_QUERY_RESULT, &elapsedTimes[2]);
glGetQueryObjectui64v(queries[3], GL_QUERY_RESULT, &elapsedTimes[3]);
glGetQueryObjectui64v(queries[4], GL_QUERY_RESULT, &elapsedTimes[4]);
glGetQueryObjectui64v(queries[5], GL_QUERY_RESULT, &elapsedTimes[5]);
glGetQueryObjectui64v(queries[6], GL_QUERY_RESULT, &elapsedTimes[6]);
glGetQueryObjectui64v(queries[7], GL_QUERY_RESULT, &elapsedTimes[7]);
glGetQueryObjectui64v(queries[8], GL_QUERY_RESULT, &elapsedTimes[8]);
glGetQueryObjectui64v(queries[9], GL_QUERY_RESULT, &elapsedTimes[9]);

float d = 1000000.0; // nanoseconds to miliseconds
tX0 += std::chrono::duration_cast<std::chrono::nanoseconds>(tEnd - tBegin).count() / d;
tX1 +=  elapsedTimes[0] / d;
tX2 +=  elapsedTimes[1] / d;
tX3 +=  elapsedTimes[2] / d;
tX4 +=  elapsedTimes[3] / d;
tX5 +=  elapsedTimes[4] / d;
tX6 +=  elapsedTimes[5] / d;
tX7 +=  elapsedTimes[6] / d;
tX8 +=  elapsedTimes[7] / d;
tX9 +=  elapsedTimes[8] / d;
tX10 += elapsedTimes[9] / d;

float TotalCPU =    tX0 / (float)frameCount;
float FillCellIds = tX1 / (float)frameCount;
float Phase1Pass1 = tX2 / (float)frameCount;
float Phase2Pass1 = tX3 / (float)frameCount;
float Phase3Pass1 = tX4 / (float)frameCount;
float Phase1Pass2 = tX5 / (float)frameCount;
float Phase2Pass2 = tX6 / (float)frameCount;
float Phase3Pass2 = tX7 / (float)frameCount;
float FindCollisionCells = tX8 / (float)frameCount;
float MouseHandling = tX9 / (float)frameCount;
float ResolveCollisions = tX10 / (float)frameCount;

int hoba = 0;
#endif
}

void Particles2dCollisionEffect::drawGUI()
{
	ImGui::Begin("Startup params (Collisions)");
	ImGui::InputInt("Particles count", &startupParams.ParticlesCount, 1000, 10000);
	ImGui::Checkbox("Turn on n-body gravity", &startupParams.IsNBodyGravity);
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

void Particles2dCollisionEffect::createComputeShaderProgram(GLuint& compShaderProgram, const char* shaderFilePath, std::vector<ShaderParams> shaderParams)
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
