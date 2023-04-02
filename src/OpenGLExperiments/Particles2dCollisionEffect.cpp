// implementation of this article
// https://developer.nvidia.com/gpugems/gpugems3/part-v-physics-simulation/chapter-32-broad-phase-collision-detection-cuda

#include <iostream>
#include "Particles2dCollisionEffect.h"

Particles2dCollisionEffect::Particles2dCollisionEffect(GLFWwindow* window)
	:AbstractEffect(window)
{
	computeShaderFilePath = "Particles2dCollisionEffect.comp";
	vertexShaderFilePath = "Particles2dCollisionEffect.vert";
	fragmentShaderFilePath = "Particles2dCollisionEffect.frag";

	startupParams = {};
	startupParams.ParticlesCount = 2;

	runtimeParams = {};
	runtimeParams.ForceScale = 1.0;
	runtimeParams.VelocityDamping = 0.999;
	runtimeParams.MinDistanceToAttractor = 20.0;
	runtimeParams.TimeScale = 1.00;
	runtimeParams.Color[0] = 1.0;
	runtimeParams.Color[1] = 1.0;
	runtimeParams.Color[2] = 1.0;
	runtimeParams.Color[3] = 1.0;
	runtimeParams.particleSize = 40.0;
	runtimeParams.cellSize = runtimeParams.particleSize;//ceil(sqrt(windowWidth * windowHeight / 65535));
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
		particles[i].PosX = random(300.0, 400.0);
		particles[i].PosY = random(300.0, 400.0);
		particles[i].VelX = random(0.0, 0.0);
	}
	//particles[0].PosX = 300;
	//particles[0].PosY = 300;
	//particles[0].VelX = 3;
	//particles[0].VelY = 0;
	//particles[1].PosX = 600;
	//particles[1].PosY = 310;
	//particles[1].VelX = 0;
	//particles[1].VelY = 0;

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

	GLuint sharedCountersLength = 12288;
	maxWorkGroupCount = 21;
	threadGroupsInWorkGroup = 48;
	threadsInThreadGroup = 4;
	threadsInWorkGroup = threadGroupsInWorkGroup * threadsInThreadGroup;
	GLuint groupCount = ceil(currentCellsCount / threadsInWorkGroup);
	phase1GroupCount = groupCount > maxWorkGroupCount ? maxWorkGroupCount : groupCount;

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
		ShaderParams {"#define cellsCount 0", "#define cellsCount " + std::to_string(currentCellsCount)}
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

	ShaderParams cellIdsLengthShaderParam          { "#define cellIdsLength 0",            "#define cellIdsLength "           + std::to_string(currentCellsCount) };
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

	ShaderParams bindingObjectIdsInputShaderParam    { "#define bindingObjectIdsInput 2",        "#define bindingObjectIdsInput 7"};
	ShaderParams bindingObjectIdsOutputShaderParam   { "#define bindingObjectIdsOutput 7",       "#define bindingObjectIdsOutput 8"};
	ShaderParams bufferObjectIdsInputShaderParam     { "#define bufferObjectIdsInput objects1",  "#define bufferObjectIdsInput objects2"};
	ShaderParams bufferObjectIdsOutputShaderParam    { "#define bufferObjectIdsOutput objects2", "#define bufferObjectIdsOutput objects3"};

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

		cellIdShiftShaderParam,
		
		bindingCellIds1ShaderParam,
		bindingCellIds2ShaderParam,
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
	std::vector<ShaderParams> findAndResolveCollisionsShaderParams
	{
		cellIdsLengthShaderParam
	};

	createComputeShaderProgram(fillCellIdAndObjectIdArraysCompShaderProgram, "FillCellIdAndObjectIdArrays.comp", fragShaderParams);
	createComputeShaderProgram(radixPhase1Pass1CompShaderProgram, "RadixSortPhase1.comp", phase1Pass1ShaderParams);
	createComputeShaderProgram(radixPhase1Pass2CompShaderProgram, "RadixSortPhase1.comp", phase1Pass2ShaderParams);
	createComputeShaderProgram(radixPhase2CompShaderProgram, "RadixSortPhase2.comp", phase2ShaderParams);
	createComputeShaderProgram(radixPhase3Pass1CompShaderProgram, "RadixSortPhase3.comp", phase3Pass1ShaderParams);
	createComputeShaderProgram(radixPhase3Pass2CompShaderProgram, "RadixSortPhase3.comp", phase3Pass2ShaderParams);
	createComputeShaderProgram(findAndResolveCollisionsCompShaderProgram, "FindAndResolveCollisions.comp", findAndResolveCollisionsShaderParams);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glGenBuffers(1, &buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 256 * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
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
	glBufferData(GL_SHADER_STORAGE_BUFFER, 1000 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Particles2dCollisionEffect::draw(GLdouble deltaTime)
{
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	//deltaTime = 0.01;
	GLfloat dt = ((GLfloat)deltaTime) * 3000 * runtimeParams.TimeScale;
GLuint* cells0;
GLuint* obj0;
	if (!isPaused || isAdvanceOneFrame)
	{
		GLdouble oldCursorPosX = cursorPosX, oldCursorPosY = cursorPosY;
		glfwGetCursorPos(window, &cursorPosX, &cursorPosY);
		if (isLeftMouseBtnDown)
		{
			
		}

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

//cells0 = readFromBuffer<GLuint>(currentCellsCount, ssboCellId);
//obj0 = readFromBuffer<GLuint>(currentCellsCount, ssboObjectId);
//GLfloat* test = readFromBuffer<GLfloat>(currentCellsCount, bufferTest);

		// ============== PHASE 1 PASS 1 =====================
		glUseProgram(radixPhase1Pass1CompShaderProgram);
		glDispatchCompute(phase1GroupCount, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		// ===================================================

		// ============== PHASE 2 PASS 1 =====================
		glUseProgram(radixPhase2CompShaderProgram);
		glDispatchCompute(64, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		// ===================================================

		// ============== PHASE 3 PASS 1 =====================
		glUseProgram(radixPhase3Pass1CompShaderProgram);
		glDispatchCompute(phase1GroupCount + 1, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		// ===================================================


		// ============== PHASE 1 PASS 2 =====================
		glUseProgram(radixPhase1Pass2CompShaderProgram);
		glDispatchCompute(phase1GroupCount, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		// ===================================================

		// ============== PHASE 2 PASS 2 =====================
		glUseProgram(radixPhase2CompShaderProgram);
		glDispatchCompute(64, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		// ===================================================

		// ============== PHASE 3 PASS 2 =====================
		glUseProgram(radixPhase3Pass2CompShaderProgram);
		glDispatchCompute(phase1GroupCount + 1, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		// ===================================================

		GLuint cellsPerThread = 10;
		GLuint threadsInBlock = 1024;
		GLuint blocks = currentCellsCount / (cellsPerThread * threadsInBlock) + 1;
		glUseProgram(findAndResolveCollisionsCompShaderProgram);

		glUniform2f(0, windowWidth, windowHeight);
		glUniform1f(6, runtimeParams.particleSize);
		glUniform1ui(7, runtimeParams.cellSize);

		glDispatchCompute(blocks, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	if (isDebug)
	{
		glUseProgram(gridShaderProgram);
		glUniform1f(0, runtimeParams.particleSize);
		glUniform2f(1, (float)cursorPosX, (float)cursorPosY);
		glUniform1i(2, windowHeight);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

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
	//glDrawArrays(GL_POINTS, currentParticlesCount * 2, 20000);
	glDrawArrays(GL_POINTS, 0, currentParticlesCount);


cells0 = readFromBuffer<GLuint>(currentCellsCount, buffer5);
obj0 = readFromBuffer<GLuint>(currentCellsCount, buffer7);
GLuint* cells = readFromBuffer<GLuint>(currentCellsCount, buffer6);
GLuint* obj = readFromBuffer<GLuint>(currentCellsCount, buffer8);
GLfloat* test = readFromBuffer<GLfloat>(1000, bufferTest);
GLuint* testInt = readFromBuffer<GLuint>(1000, bufferTest);
Particle* particles = readFromBuffer<Particle>(currentParticlesCount, ssboParticles);

//double mx, my;
//glfwGetCursorPos(window, &mx, &my);
//int objIdUnderMouse = -1;
//int collisions = 0;
for (int i = 0; i < currentParticlesCount; i++)
{
	if (isnan(particles[i].PosX))
	{
		int l = 0;
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

	isAdvanceOneFrame = false;
	isLeftMouseBtnPressed = false;
	isLeftMouseBtnReleased = false;
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
