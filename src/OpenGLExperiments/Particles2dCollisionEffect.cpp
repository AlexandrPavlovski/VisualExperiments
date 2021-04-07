#include "Particles2dCollisionEffect.h"

Particles2dCollisionEffect::Particles2dCollisionEffect(GLFWwindow* window)
	:AbstractEffect(window)
{
	computeShaderFilePath = "Particles2dCollisionEffect.comp";
	vertexShaderFilePath = "Particles2dCollisionEffect.vert";
	fragmentShaderFilePath = "Particles2dCollisionEffect.frag";

	startupParams = {};
	startupParams.ParticlesCount = 100000;

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
	srand(1);
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


	std::vector<GLushort> cellIds(startupParams.ParticlesCount * 4+10, 0);

	glGenVertexArrays(1, &vaoCellId);
	glBindVertexArray(vaoCellId);

	glGenBuffers(1, &ssboCellId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCellId);
	glBufferData(GL_SHADER_STORAGE_BUFFER, cellIds.size() * sizeof(GLushort), &cellIds[0], GL_DYNAMIC_DRAW | GL_DYNAMIC_READ);
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
	groupCount = ceil(startupParams.ParticlesCount * 2 / threadsInWorkGroup);
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
		ShaderParams {"#define particlesCountDoubled 0", "#define particlesCountDoubled " + std::to_string(currentParticlesCount * 2)}
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

	
	cellIdsLength = currentParticlesCount * 2; // each cell id is 2 bytes. cellIds is array of 4 byte values because glsl doesn't have 2-byte type
	elementsPerThread = ceil(cellIdsLength / (phase1GroupCount * threadsInWorkGroup));
	elementsPerGroup = threadsInThreadGroup * elementsPerThread;
	threadGroupsTotal = ceil(cellIdsLength / elementsPerGroup);

	ShaderParams cellIdsLengthShaderParam          { "#define cellIdsLength 0",           "#define cellIdsLength "           + std::to_string(cellIdsLength) };
	ShaderParams threadsInWorkGroupShaderParam     { "#define threadsInWorkGroup 1",      "#define threadsInWorkGroup "      + std::to_string((int)threadsInWorkGroup) };
	ShaderParams threadGroupsInWorkGroupShaderParam{ "#define threadGroupsInWorkGroup 0", "#define threadGroupsInWorkGroup " + std::to_string(threadGroupsInWorkGroup) };
	ShaderParams threadsInThreadGroupShaderParam   { "#define threadsInThreadGroup 0",    "#define threadsInThreadGroup "    + std::to_string(threadsInThreadGroup) };
	ShaderParams elementsPerGroupShaderParam       { "#define elementsPerGroup 0",        "#define elementsPerGroup "        + std::to_string(elementsPerGroup) };
	//ShaderParams elementsPerThreadShaderParam      { "#define elementsPerThread 0",       "#define elementsPerThread "       + std::to_string(elementsPerThread) };
	ShaderParams threadGroupsTotalShaderParam      { "#define threadGroupsTotal 1",       "#define threadGroupsTotal "       + std::to_string(threadGroupsTotal) };

	std::vector<ShaderParams> phase1ShaderParams
	{
		cellIdsLengthShaderParam,
		threadsInWorkGroupShaderParam,
		threadGroupsInWorkGroupShaderParam,
		threadsInThreadGroupShaderParam,
		//elementsPerThreadShaderParam,
		elementsPerGroupShaderParam
	};
	std::vector<ShaderParams> phase2ShaderParams
	{
		threadGroupsInWorkGroupShaderParam,
		threadsInThreadGroupShaderParam,
		elementsPerGroupShaderParam,
		threadGroupsTotalShaderParam
	};
	//Particles2dCollisionEffect
	//FillCellIdAndObjectIdArrays
	createComputeShaderProgram(fillCellIdAndObjectIdArraysCompShaderProgram, "FillCellIdAndObjectIdArrays.comp", fragShaderParams);
	createComputeShaderProgram(radixPhase1CompShaderProgram, "RadixSortPhase1.comp", phase1ShaderParams);
	createComputeShaderProgram(radixPhase2CompShaderProgram, "RadixSortPhase2.comp", phase2ShaderParams);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGenBuffers(1, &buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 256 * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Particles2dCollisionEffect::draw(GLdouble deltaTime)
{
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

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

	//glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCellId);
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellId);
	//GLushort r[400010] = { 0 };
	//GLushort* ptr = (GLushort*)glMapNamedBufferRange(ssboCellId, 0, currentParticlesCount * 4 * sizeof(GLushort)+10, GL_MAP_READ_BIT);
	//GLenum e = glGetError();
	//memcpy(r, ptr, sizeof(GLushort) * currentParticlesCount * 4 + 10);
	//bool result = glUnmapNamedBuffer(ssboCellId);

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


	glUseProgram(radixPhase1CompShaderProgram);

	glUniform2f(0, windowWidth, windowHeight);
	glUniform1f(1, runtimeParams.ForceScale);
	glUniform1f(2, runtimeParams.VelocityDamping);
	glUniform1f(3, runtimeParams.MinDistanceToAttractor);
	glUniform1f(4, dt);
	glUniform1i(5, isPaused && !isAdvanceOneFrame);
	glUniform1f(6, runtimeParams.particleSize);
	runtimeParams.cellSize = runtimeParams.particleSize;
	glUniform1ui(7, runtimeParams.cellSize);

	glDispatchCompute(phase1GroupCount, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

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

	// ===================================================
	glUseProgram(radixPhase2CompShaderProgram);

	glUniform2f(0, windowWidth, windowHeight);
	glUniform1f(1, runtimeParams.ForceScale);
	glUniform1f(2, runtimeParams.VelocityDamping);
	glUniform1f(3, runtimeParams.MinDistanceToAttractor);
	glUniform1f(4, dt);
	glUniform1i(5, isPaused && !isAdvanceOneFrame);
	glUniform1f(6, runtimeParams.particleSize);
	runtimeParams.cellSize = runtimeParams.particleSize;
	glUniform1ui(7, runtimeParams.cellSize);

	GLuint groupCount3 = 64;
	glDispatchCompute(groupCount3, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	// ===================================================

	
	GLuint presummOfTotalSumms[256] = { 0 };
	GLuint* p = (GLuint*)glMapNamedBufferRange(buffer, 0, 256 * sizeof(GLuint), GL_MAP_READ_BIT);
	GLenum e = glGetError();
	memcpy(presummOfTotalSumms, p, sizeof(GLuint) * 256);
	bool result = glUnmapNamedBuffer(buffer);
	
	for (int i = 0; i < 256; i += 4)
	{
		for (int j = 1; j < 4; j++)
		{
			if (presummOfTotalSumms[j - 1 + i] > presummOfTotalSumms[j + i])
			{
				int r = 0;
			}
		}
	}


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
