#include "Particles2dCollisionEffect.h"

Particles2dCollisionEffect::Particles2dCollisionEffect(GLFWwindow* window)
	:AbstractEffect(window)
{
	computeShaderFilePath = "Particles2dCollisionEffect.comp";
	vertexShaderFilePath = "Particles2dCollisionEffect.vert";
	fragmentShaderFilePath = "Particles2dCollisionEffect.frag";

	startupParams = {};
	startupParams.ParticlesCount = 8000;

	runtimeParams = {};
	runtimeParams.ForceScale = 1.0;
	runtimeParams.VelocityDamping = 0.999;
	runtimeParams.MinDistanceToAttractor = 20.0;
	runtimeParams.TimeScale = 1.00;
	runtimeParams.Color[0] = 1.0;
	runtimeParams.Color[1] = 1.0;
	runtimeParams.Color[2] = 1.0;
	runtimeParams.Color[3] = 1.0;
	runtimeParams.particleSize = 25.0;
	runtimeParams.cellSize = runtimeParams.particleSize;//ceil(sqrt(windowWidth * windowHeight / 65535));
}

Particles2dCollisionEffect::~Particles2dCollisionEffect()
{
	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &ssbo);
	glDeleteVertexArrays(1, &vao);
}


void Particles2dCollisionEffect::initialize()
{
	srand(101);
	particlesData = std::vector<GLfloat>(startupParams.ParticlesCount * 8 + 65535 * 4, 0.0f);
	int j = 0;
	for (int i = 0; i < startupParams.ParticlesCount; i++)
	{
		// positions
		//particlesData.push_back(50.5);
		//particlesData.push_back(50.5);
		particlesData[j++] = random(100.0, 700.0);
		particlesData[j++] = random(100.0, 700.0);
		// velocities
		//particlesData.push_back(0.0);
		//particlesData.push_back(0.0);
		particlesData[j++] = random(-0.3, 0.3);
		particlesData[j++] = random(-0.3, 0.3);
	}

	//particlesData.push_back(10.0);particlesData.push_back(10.0);particlesData.push_back(0.0);particlesData.push_back(0.0);
	//particlesData.push_back(10.0);particlesData.push_back(30.0);particlesData.push_back(0.0);particlesData.push_back(0.0);
	//particlesData.push_back(10.0);particlesData.push_back(50.0);particlesData.push_back(0.0);particlesData.push_back(0.0);
	//particlesData.push_back(10.0);particlesData.push_back(70.0);particlesData.push_back(0.0);particlesData.push_back(0.0);
	//particlesData.push_back(10.0);particlesData.push_back(90.0);particlesData.push_back(0.0);particlesData.push_back(0.0);
	//particlesData.push_back(10.0);particlesData.push_back(110.0);particlesData.push_back(0.0);particlesData.push_back(0.0);
	//particlesData.push_back(10.0);particlesData.push_back(130.0);particlesData.push_back(0.0);particlesData.push_back(0.0);
	//particlesData.push_back(10.0);particlesData.push_back(150.0);particlesData.push_back(0.0);particlesData.push_back(0.0);
	//particlesData.push_back(10.0);particlesData.push_back(170.0);particlesData.push_back(0.0);particlesData.push_back(0.0);
	//particlesData.push_back(10.0);particlesData.push_back(190.0);particlesData.push_back(0.0);particlesData.push_back(0.0);

	//for (int i = 0; i < startupParams.ParticlesCount; i++)
	//{
	//	// pressure
	//	particlesData[j] = 0.0;
	//	// acceleration
	//	particlesData[j] = 0.0;
	//	particlesData[j] = 0.0;
	//	// unused
	//	particlesData[j] = 0.0;
	//}
	//// used for grid rendering
	//for (int i = 0; i < 65535; i++)
	//{
	//	particlesData[j] = 0.0;
	//	particlesData[j] = 0.0;
	//	particlesData[j] = 0.0;
	//	particlesData[j] = 0.0;
	//}


	//particlesData.push_back(700.0);
	//particlesData.push_back(410.0);
	//particlesData.push_back(5.0);
	//particlesData.push_back(0.0);

	//particlesData.push_back(800.0);
	//particlesData.push_back(400.0);
	//particlesData.push_back(-3.0);
	//particlesData.push_back(0.0);

	//particlesData.push_back(200.0);
	//particlesData.push_back(500.0);
	//particlesData.push_back(0.0);
	//particlesData.push_back(0.0);

	//particlesData.push_back(300.0);
	//particlesData.push_back(500.0);
	//particlesData.push_back(0.0);
	//particlesData.push_back(0.0);

	//particlesData.push_back(0.0);particlesData.push_back(0.0);particlesData.push_back(0.0);particlesData.push_back(0.0);
	//particlesData.push_back(0.0);particlesData.push_back(0.0);particlesData.push_back(0.0);particlesData.push_back(0.0);
	//particlesData.push_back(0.0);particlesData.push_back(0.0);particlesData.push_back(0.0);particlesData.push_back(0.0);
	//particlesData.push_back(0.0);particlesData.push_back(0.0);particlesData.push_back(0.0);particlesData.push_back(0.0);


	GLuint vaoCellId = 0;//, ssboCellId = 0;
	GLuint vaoObjectId = 0;//, ssboObjectId = 0;
	GLuint vaoGlobalCounters = 0;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, particlesData.size() * sizeof(GLfloat), &particlesData[0], GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	std::vector<GLuint> cellIds(startupParams.ParticlesCount * 4+100, 0);

	glGenVertexArrays(1, &vaoCellId);
	glBindVertexArray(vaoCellId);

	glGenBuffers(1, &ssboCellId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCellId);
	glBufferData(GL_SHADER_STORAGE_BUFFER, cellIds.size() * sizeof(GLuint), &cellIds[0], GL_DYNAMIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	std::vector<GLuint> objectIds(startupParams.ParticlesCount * 4, 1);

	glGenVertexArrays(1, &vaoObjectId);
	glBindVertexArray(vaoObjectId);

	glGenBuffers(1, &ssboObjectId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboObjectId);
	glBufferData(GL_SHADER_STORAGE_BUFFER, objectIds.size() * sizeof(GLuint), &objectIds[0], GL_DYNAMIC_DRAW | GL_DYNAMIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboObjectId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	maxWorkGroupCount = 21;
	threadGroupsInWorkGroup = 48;
	threadsInThreadGroup = 4;
	threadsInWorkGroup = threadGroupsInWorkGroup * threadsInThreadGroup;
	GLuint groupCount = ceil(startupParams.ParticlesCount * 4 / threadsInWorkGroup);
	phase1GroupCount = groupCount > maxWorkGroupCount ? maxWorkGroupCount : groupCount;

	std::vector<GLuint> globalCounters(12288 * phase1GroupCount, 0);

	glGenVertexArrays(1, &vaoGlobalCounters);
	glBindVertexArray(vaoGlobalCounters);

	glGenBuffers(1, &ssboGlobalCounters);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboGlobalCounters);
	glBufferData(GL_SHADER_STORAGE_BUFFER, globalCounters.size() * sizeof(GLuint), &globalCounters[0], GL_DYNAMIC_DRAW | GL_DYNAMIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboGlobalCounters);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	particlesData.clear();
	currentParticlesCount = startupParams.ParticlesCount;

	std::vector<ShaderParams> vertShaderParams
	{
		ShaderParams {"#define particlesCount 0", "#define particlesCount " + std::to_string(currentParticlesCount)},
		ShaderParams {"#define particlesCountTimes4 0", "#define particlesCountTimes4 " + std::to_string(currentParticlesCount * 4)}
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

	
	cellIdsLength = currentParticlesCount * 4;
	elementsPerThread = ceil(cellIdsLength / (phase1GroupCount * threadsInWorkGroup));
	elementsPerGroup = threadsInThreadGroup * elementsPerThread;
	threadGroupsTotal = ceil(cellIdsLength / (double)elementsPerGroup);

	ShaderParams cellIdsLengthShaderParam          { "#define cellIdsLength 0",            "#define cellIdsLength "           + std::to_string(cellIdsLength) };
	ShaderParams threadsInWorkGroupShaderParam     { "#define threadsInWorkGroup 1",       "#define threadsInWorkGroup "      + std::to_string((int)threadsInWorkGroup) };
	ShaderParams threadGroupsInWorkGroupShaderParam{ "#define threadGroupsInWorkGroup 1",  "#define threadGroupsInWorkGroup " + std::to_string(threadGroupsInWorkGroup) };
	ShaderParams threadsInThreadGroupShaderParam   { "#define threadsInThreadGroup 0",     "#define threadsInThreadGroup "    + std::to_string(threadsInThreadGroup) };
	ShaderParams elementsPerGroupShaderParam       { "#define elementsPerGroup 0",         "#define elementsPerGroup "        + std::to_string(elementsPerGroup) };
	ShaderParams threadGroupsTotalShaderParam      { "#define threadGroupsTotal 1",        "#define threadGroupsTotal "       + std::to_string(threadGroupsTotal) };
	ShaderParams bindingCellIds1ShaderParam        { "#define bindingCellIds1 1",          "#define bindingCellIds1 5"};
	ShaderParams bindingCellIds2ShaderParam        { "#define bindingCellIds2 5",          "#define bindingCellIds2 1"};
	ShaderParams cellIdShiftShaderParam            { "#define cellIdShift 0",              "#define cellIdShift 8"};
	ShaderParams cells1ShaderParam                 { "#define cellsFirst cells1",          "#define cellsFirst cells2"};
	ShaderParams cells2ShaderParam                 { "#define cellsSecond cells2",         "#define cellsSecond cells1"};

	ShaderParams bindingCellIdsInputShaderParam    { "#define bindingCellIdsInput 1",      "#define bindingCellIdsInput 5"};
	ShaderParams bindingCellIdsOutputShaderParam   { "#define bindingCellIdsOutput 5",     "#define bindingCellIdsOutput 6"};
	ShaderParams bufferCellIdsInputShaderParam     { "#define bufferCellIdsInput cells1",  "#define bufferCellIdsInput cells2"};
	ShaderParams bufferCellIdsOutputShaderParam    { "#define bufferCellIdsOutput cells2", "#define bufferCellIdsOutput cells3"};

	std::vector<ShaderParams> phase1Pass1ShaderParams
	{
		cellIdsLengthShaderParam,
		threadsInWorkGroupShaderParam,
		threadGroupsInWorkGroupShaderParam,
		threadsInThreadGroupShaderParam,
		elementsPerGroupShaderParam
	};
	std::vector<ShaderParams> phase1Pass2ShaderParams
	{
		cellIdsLengthShaderParam,
		threadsInWorkGroupShaderParam,
		threadGroupsInWorkGroupShaderParam,
		threadsInThreadGroupShaderParam,
		elementsPerGroupShaderParam,
		bindingCellIds1ShaderParam,
		bindingCellIds2ShaderParam,
		cellIdShiftShaderParam,
		cells1ShaderParam,
		cells2ShaderParam
	};
	std::vector<ShaderParams> phase2ShaderParams
	{
		threadGroupsInWorkGroupShaderParam,
		threadsInThreadGroupShaderParam,
		elementsPerGroupShaderParam,
		threadGroupsTotalShaderParam
	};
	std::vector<ShaderParams> phase3Pass1ShaderParams
	{
		cellIdsLengthShaderParam,
		threadsInWorkGroupShaderParam,
		threadGroupsInWorkGroupShaderParam,
		threadsInThreadGroupShaderParam,
		elementsPerGroupShaderParam
	};
	std::vector<ShaderParams> phase3Pass2ShaderParams
	{
		cellIdsLengthShaderParam,
		threadsInWorkGroupShaderParam,
		threadGroupsInWorkGroupShaderParam,
		threadsInThreadGroupShaderParam,
		elementsPerGroupShaderParam,
		bindingCellIdsInputShaderParam,
		bindingCellIdsOutputShaderParam,
		cellIdShiftShaderParam,
		bufferCellIdsInputShaderParam,
		bufferCellIdsOutputShaderParam
	};

	createComputeShaderProgram(fillCellIdAndObjectIdArraysCompShaderProgram, "FillCellIdAndObjectIdArrays.comp", fragShaderParams);
	createComputeShaderProgram(radixPhase1Pass1CompShaderProgram, "RadixSortPhase1.comp", phase1Pass1ShaderParams);
	createComputeShaderProgram(radixPhase1Pass2CompShaderProgram, "RadixSortPhase1.comp", phase1Pass2ShaderParams);
	createComputeShaderProgram(radixPhase2CompShaderProgram, "RadixSortPhase2.comp", phase2ShaderParams);
	createComputeShaderProgram(radixPhase3Pass1CompShaderProgram, "RadixSortPhase3.comp", phase3Pass1ShaderParams);
	createComputeShaderProgram(radixPhase3Pass2CompShaderProgram, "RadixSortPhase3.comp", phase3Pass2ShaderParams);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glGenBuffers(1, &buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 256 * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &buffer2);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buffer2);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 800000 * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &buffer3);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buffer3);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 400000 * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &buffer7);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, buffer7);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 40000 * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Particles2dCollisionEffect::draw(GLdouble deltaTime)
{
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	deltaTime = 0.01666666 * 2;
	GLfloat dt = ((GLfloat)deltaTime) * 1000 * runtimeParams.TimeScale;

	glUseProgram(fillCellIdAndObjectIdArraysCompShaderProgram);

	glUniform2f(0, windowWidth, windowHeight);
	glUniform1f(1, runtimeParams.ForceScale);
	glUniform1f(2, runtimeParams.VelocityDamping);
	glUniform1f(3, runtimeParams.MinDistanceToAttractor);
	glUniform1f(4, dt);
	glUniform1i(5, isPaused && !isAdvanceOneFrame);
	glUniform1f(6, runtimeParams.particleSize);
	runtimeParams.cellSize = runtimeParams.particleSize;
	glUniform1ui(7, runtimeParams.cellSize);

	GLuint groupCount = ceil(currentParticlesCount / 64.0f);
	glDispatchCompute(groupCount, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//GLushort r[400010] = { 0 };
	//GLushort* ptr0 = (GLushort*)glMapNamedBufferRange(ssboCellId, 0, currentParticlesCount * 4 * sizeof(GLushort)+10, GL_MAP_READ_BIT);
	//GLenum e0 = glGetError();
	//memcpy(r, ptr0, sizeof(GLushort) * currentParticlesCount * 4 + 10);
	//bool result0 = glUnmapNamedBuffer(ssboCellId);

	//short counters[256] = { 0 };
	//for (int i = 0; i < 400000; i++)
	//{
	//	short radix = r[i] & 255;
	//	counters[radix]++;
	//}
	//int tot = 0;
	//for (int i = 0; i < 256; i++)
	//{
	//	tot += counters[i];
	//}

	//GLushort t1 = r[400009];
	//GLushort t2 = r[400008];
	//GLushort t3 = r[400007];
	//GLushort t4 = r[400006];
	//GLushort t5 = r[400005];
	//GLushort t6 = r[400004];
	//GLushort t7 = r[400003];
	//GLushort t8 = r[400002];
	//GLushort t9 = r[400001];
	//GLushort t10 = r[400000];
	//GLushort t11 = r[399999];

	//GLint r2[500] = { 0 };
	//GLuint* ptr2 = (GLuint*)glMapNamedBufferRange(ssboObjectId, 0, currentParticlesCount * 4 * sizeof(GLuint), GL_MAP_READ_BIT);
	//GLenum e2 = glGetError();
	//memcpy(r2, ptr2, sizeof(GLuint) * 500);
	//bool result2 = glUnmapNamedBuffer(ssboObjectId);


	// ============== PHASE 1 ============================
	glUseProgram(radixPhase1Pass1CompShaderProgram);
	glDispatchCompute(phase1GroupCount, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	// ===================================================

	//GLuint cellsCount = currentParticlesCount * 4;
	//GLushort r[400010] = { 0 };
	//GLushort* ptr = (GLushort*)glMapNamedBufferRange(ssboCellId, 0, cellsCount * sizeof(GLushort), GL_MAP_READ_BIT);
	//GLenum e = glGetError();
	//memcpy(r, ptr, sizeof(GLushort) * cellsCount);
	//bool result = glUnmapNamedBuffer(ssboCellId);

	//GLushort t1 = r[400009];
	//GLushort t2 = r[400008];
	//GLushort t3 = r[400007];
	//GLushort t4 = r[400006];
	//GLushort t5 = r[400005];
	//GLushort t6 = r[400004];
	//GLushort t7 = r[400003];
	//GLushort t8 = r[400002];
	//GLushort t9 = r[400001];
	//GLushort t10 = r[400000];
	//GLushort t11 = r[399999];

	//short counters[256] = { 0 };
	//for (int i = 0; i < cellsCount; i++)
	//{
	//	short radix = r[i] & 255;
	//	counters[radix]++;
	//}
	//int tot = 0;
	//for (int i = 0; i < 256; i++)
	//{
	//	tot += counters[i];
	//}

	//int total = 0;
	//short counters2[256] = { 0 };
	//for (int o = 0; o < phase1GroupCount; o++)
	//{
	//	GLint r3[12288] = { 0 };
	//	GLuint* ptr3 = (GLuint*)glMapNamedBufferRange(ssboGlobalCounters, 12288 * o * sizeof(GLuint), 12288 * sizeof(GLuint), GL_MAP_READ_BIT);
	//	GLenum e3 = glGetError();
	//	memcpy(r3, ptr3, sizeof(GLuint) * 12288);
	//	for (int i = 0; i < 12288; i++)
	//	{
	//		total += r3[i];
	//		counters2[i % 256] += r3[i];
	//	}
	//	bool result3 = glUnmapNamedBuffer(ssboGlobalCounters);
	//}

	//int totErr = 0;
	//int errors[256] = { 0 };
	//for (int i = 0; i < 256; i++)
	//{
	//	errors[i] = counters[i] - counters2[i];
	//	totErr += counters[i] - counters2[i];
	//}


	//if (total != currentParticlesCount * 4)
	//{
	//	int t = 0;
	//}

	//GLuint* globalCountersBefore = new GLuint[12288 * 21];
	//GLuint* ptr3 = (GLuint*)glMapNamedBufferRange(ssboGlobalCounters, 0, 12288 * phase1GroupCount * sizeof(GLuint), GL_MAP_READ_BIT);
	//GLenum e3 = glGetError();
	//memcpy(globalCountersBefore, ptr3, sizeof(GLuint) * 12288 * phase1GroupCount);
	//bool result3 = glUnmapNamedBuffer(ssboGlobalCounters);

	// ============== PHASE 2 ============================
	glUseProgram(radixPhase2CompShaderProgram);
	glDispatchCompute(64, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	// ===================================================


	//GLuint totalSumms[256] = { 0 };
	//GLuint* p = (GLuint*)glMapNamedBufferRange(buffer, 0, 256 * sizeof(GLuint), GL_MAP_READ_BIT);
	//GLenum e = glGetError();
	//memcpy(totalSumms, p, sizeof(GLuint) * 256);
	//bool result = glUnmapNamedBuffer(buffer);
	//
	//for (int i = 0; i < 256; i += 4)
	//{
	//	for (int j = 1; j < 4; j++)
	//	{
	//		if (totalSumms[j - 1 + i] > totalSumms[j + i])
	//		{
	//			int r = 0;
	//		}
	//	}
	//}

	//GLuint* globalCountersAfter = new GLuint[12288 * 21];
	//GLuint* ptr4 = (GLuint*)glMapNamedBufferRange(ssboGlobalCounters, 0, 12288 * phase1GroupCount * sizeof(GLuint), GL_MAP_READ_BIT);
	//GLenum e4 = glGetError();
	//memcpy(globalCountersAfter, ptr4, sizeof(GLuint) * 12288 * phase1GroupCount);
	//bool result4 = glUnmapNamedBuffer(ssboGlobalCounters);
	//
	//for (int i = 0; i < 12288 * phase1GroupCount; i++)
	//{
	//	if (globalCountersBefore[i] != globalCountersAfter[i])
	//	{
	//		int r = 0;
	//	}
	//}

	//GLuint presummOfTotalSummsAf[256] = { 0 };
	//GLuint totSumm = 0;
	//for (int radix = 0; radix < 256; radix++)
	//{
	//	std::vector<GLuint> presumOneRadixBef(threadGroupsTotal, 0);
	//	std::vector<GLuint> presumOneRadixAf(threadGroupsTotal, 0);

	//	GLuint summ = 0;
	//	GLuint t = 0;
	//	for (int i = 0; i < threadGroupsTotal; i++)
	//	{
	//		t = globalCountersBefore[i * 256 + radix];
	//		presumOneRadixBef[i] = summ;
	//		summ += t;

	//		presumOneRadixAf[i] = globalCountersAfter[i * 256 + radix];
	//	}

	//	for (int i = 0; i < threadGroupsTotal; i++)
	//	{
	//		if (presumOneRadixBef[i] != presumOneRadixAf[i])
	//		{
	//			int r = 0;
	//		}
	//	}

	//	if (radix % 4 == 0)
	//		totSumm = 0;
	//	presummOfTotalSummsAf[radix] = totSumm;
	//	totSumm += presumOneRadixAf[threadGroupsTotal - 1] + t;
	//}
	//for (int i = 0; i < 256; i++)
	//{
	//	if (presummOfTotalSumms[i] != presummOfTotalSummsAf[i])
	//	{
	//		int r = 0;
	//	}
	//}
	//delete[] globalCountersBefore;
	//delete[] globalCountersAfter;

	// ============== PHASE 3 ============================
	glUseProgram(radixPhase3Pass1CompShaderProgram);
	glDispatchCompute(phase1GroupCount + 1, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	// ===================================================

	//GLuint presummOfTotalSumms[256] = { 0 };
	//GLuint* p = (GLuint*)glMapNamedBufferRange(buffer, 0, 256 * sizeof(GLuint), GL_MAP_READ_BIT);
	//GLenum e = glGetError();
	//memcpy(presummOfTotalSumms, p, sizeof(GLuint) * 256);
	//bool result = glUnmapNamedBuffer(buffer);

	//GLuint* globalCountersAfter = new GLuint[12288 * 21];
	//GLuint* ptr4 = (GLuint*)glMapNamedBufferRange(ssboGlobalCounters, 0, 12288 * 21 * sizeof(GLuint), GL_MAP_READ_BIT);
	//GLenum e4 = glGetError();
	//memcpy(globalCountersAfter, ptr4, sizeof(GLuint) * 12288 * 21);
	//bool result4 = glUnmapNamedBuffer(ssboGlobalCounters);

	//for (int radix = 0; radix < 256; radix++)
	//{
	//	std::vector<GLuint> presumOneRadixAf(threadGroupsTotal, 0);

	//	GLuint summ = 0;
	//	GLuint t = 0;
	//	for (int i = 0; i < threadGroupsTotal; i++)
	//	{
	//		presumOneRadixAf[i] = globalCountersAfter[i * 256 + radix];
	//	}

	//	for (int i = 1; i < threadGroupsTotal; i++)
	//	{
	//		if (presumOneRadixAf[i - 1] > presumOneRadixAf[i])
	//		{
	//			int r = 0;
	//		}
	//	}
	//}

	int len = currentParticlesCount * 4;
	GLuint* buf2 = new GLuint[len];
	GLuint* p23 = (GLuint*)glMapNamedBufferRange(buffer2, 0, len * sizeof(GLuint), GL_MAP_READ_BIT);
	memcpy(buf2, p23, len * sizeof(GLuint));
	glUnmapNamedBuffer(buffer2);

	//GLuint* bufce = new GLuint[len];
	//GLuint* p23w = (GLuint*)glMapNamedBufferRange(ssboCellId, 0, len * sizeof(GLuint), GL_MAP_READ_BIT);
	//memcpy(bufce, p23w, len * sizeof(GLuint));
	//glUnmapNamedBuffer(ssboCellId);

	//GLuint* buf7 = new GLuint[40000];
	//GLuint* p7 = (GLuint*)glMapNamedBufferRange(buffer7, 0, 40000 * sizeof(GLuint), GL_MAP_READ_BIT);
	//memcpy(buf7, p7, 40000 * sizeof(GLuint));
	//glUnmapNamedBuffer(buffer7);

	for (int i = 1; i < currentParticlesCount * 4; i++)
	{
		if ((buf2[i - 1] & 255) > (buf2[i] & 255))
		{
			int nt = 0;
		}
	}
	// ============== PHASE 1 ============================
	glUseProgram(radixPhase1Pass2CompShaderProgram);
	glDispatchCompute(phase1GroupCount, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	// ===================================================

	//int total = 0;
	//short counters2[256] = { 0 };
	//GLint r3[12288] = { 0 };
	//GLuint* ptr3 = (GLuint*)glMapNamedBufferRange(ssboGlobalCounters, 0, 12288 * sizeof(GLuint), GL_MAP_READ_BIT);
	//memcpy(r3, ptr3, sizeof(GLuint) * 12288);
	//for (int i = 0; i < 12288; i++)
	//{
	//	total += r3[i];
	//	counters2[i % 256] += r3[i];
	//}
	//glUnmapNamedBuffer(ssboGlobalCounters);

	// ============== PHASE 2 ============================
	glUseProgram(radixPhase2CompShaderProgram);
	glDispatchCompute(64, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	// ===================================================
	
	// ============== PHASE 3 ============================
	glUseProgram(radixPhase3Pass2CompShaderProgram);
	glDispatchCompute(phase1GroupCount + 1, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	// ===================================================

	delete[] buf2;
	buf2 = new GLuint[len];
	p23 = (GLuint*)glMapNamedBufferRange(buffer3, 0, len * sizeof(GLuint), GL_MAP_READ_BIT);
	memcpy(buf2, p23, len * sizeof(GLuint));
	glUnmapNamedBuffer(buffer3);

	//GLuint* buf7 = new GLuint[40000];
	//GLuint* p7 = (GLuint*)glMapNamedBufferRange(buffer7, 0, 40000 * sizeof(GLuint), GL_MAP_READ_BIT);
	//memcpy(buf7, p7, 40000 * sizeof(GLuint));
	//glUnmapNamedBuffer(buffer7);

	for (int i = 1; i < currentParticlesCount * 4; i++)
	{
		if (buf2[i - 1] > buf2[i])
		{
			int nt = 0;
		}
	}

	//frame++;
	//delete[] buf;
	delete[] buf2;


	glUseProgram(shaderProgram);

	// vertex shader params
	glUniform2f(0, windowWidth, windowHeight);
	glUniform1f(1, runtimeParams.ForceScale);
	glUniform1f(2, runtimeParams.VelocityDamping);
	glUniform1f(3, runtimeParams.MinDistanceToAttractor);
	glUniform1f(4, dt);
	glUniform1i(5, isPaused && !isAdvanceOneFrame);
	glUniform1f(6, runtimeParams.particleSize);
	glUniform1ui(7, runtimeParams.cellSize);
	// fragment shader params
	glUniform4f(20, runtimeParams.Color[0], runtimeParams.Color[1], runtimeParams.Color[2], runtimeParams.Color[3]);

	glPointSize(runtimeParams.particleSize);
	glDrawArrays(GL_POINTS, currentParticlesCount * 2, 20000);
	glDrawArrays(GL_POINTS, 0, currentParticlesCount);


	isAdvanceOneFrame = false;
}

void Particles2dCollisionEffect::drawGUI()
{
	ImGui::Begin("Startup params (Collisions)");
	ImGui::InputInt("Particles count", &startupParams.ParticlesCount, 1000, 10000);
	ImGui::End();

	ImGui::Begin("Runtime params (Collisions)");
	ImGui::SliderFloat("Force scale", &runtimeParams.ForceScale, -1.0, 10.0);
	ImGui::SliderFloat("Velocity damping", &runtimeParams.VelocityDamping, 0.9, 1.0);
	ImGui::SliderFloat("Min distance", &runtimeParams.MinDistanceToAttractor, 0.01, 100.0);
	ImGui::SliderFloat("Time scale", &runtimeParams.TimeScale, 0.0, 5.0);
	ImGui::SliderFloat("Size", &runtimeParams.particleSize, 1.0, 500.0);
	ImGui::End();
}

void Particles2dCollisionEffect::restart()
{
	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &ssbo);
	glDeleteVertexArrays(1, &vao);

	initialize();
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
