#include "Particles2dCollisionEffect.h"

Particles2dCollisionEffect::Particles2dCollisionEffect(GLFWwindow* window)
	:AbstractEffect(window)
{
	computeShaderFilePath = "Particles2dCollisionEffect.comp";
	vertexShaderFilePath = "Particles2dCollisionEffect.vert";
	fragmentShaderFilePath = "Particles2dCollisionEffect.frag";

	startupParams = {};
	startupParams.ParticlesCount = 50;

	runtimeParams = {};
	runtimeParams.ForceScale = 1.0;
	runtimeParams.VelocityDamping = 0.999;
	runtimeParams.MinDistanceToAttractor = 20.0;
	runtimeParams.TimeScale = 1.00;
	runtimeParams.Color[0] = 1.0;
	runtimeParams.Color[1] = 1.0;
	runtimeParams.Color[2] = 1.0;
	runtimeParams.Color[3] = 1.0;
	runtimeParams.particleSize = 20.0;
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
	for (int i = 0; i < startupParams.ParticlesCount; i++)
	{
		// positions
		particlesData.push_back(random(100.0, 700.0));
		particlesData.push_back(random(100.0, 700.0));
		// velocities
		particlesData.push_back(random(-0.5, 0.5));
		particlesData.push_back(random(-0.5, 0.5));
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

	for (int i = 0; i < startupParams.ParticlesCount; i++)
	{
		// pressure
		particlesData.push_back(0.0);
		// acceleration
		particlesData.push_back(0.0);
		particlesData.push_back(0.0);
		// unused
		particlesData.push_back(0.0);
	}
	for (int i = 0; i < 65535; i++)
	{
		particlesData.push_back(0.0);
		particlesData.push_back(0.0);
		particlesData.push_back(0.0);
		particlesData.push_back(0.0);
	}


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

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, particlesData.size() * sizeof(GLfloat), &particlesData[0], GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	std::vector<GLushort> cellIds(startupParams.ParticlesCount * 4, 0);

	glGenVertexArrays(1, &vaoCellId);
	glBindVertexArray(vaoCellId);

	glGenBuffers(1, &ssboCellId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCellId);
	glBufferData(GL_SHADER_STORAGE_BUFFER, cellIds.size() * sizeof(GLushort), &cellIds[0], GL_DYNAMIC_DRAW | GL_DYNAMIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	std::vector<GLint> objectIds(startupParams.ParticlesCount * 4, 0);

	glGenVertexArrays(1, &vaoObjectId);
	glBindVertexArray(vaoObjectId);

	glGenBuffers(1, &ssboObjectId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboObjectId);
	glBufferData(GL_SHADER_STORAGE_BUFFER, objectIds.size() * sizeof(GLint), &objectIds[0], GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboObjectId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	particlesData.clear();
	currentParticlesCount = startupParams.ParticlesCount;

	std::vector<ShaderParams> vertShaderParams
	{
		ShaderParams {"#define particlesCount 0", "#define particlesCount " + std::to_string(currentParticlesCount)}
	};
	GLint newShaderProgram = createShaderProgramFromFiles(vertShaderParams, vertShaderParams);
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}

	shaderProgram = newShaderProgram;
	//Particles2dCollisionEffect
	//FillCellIdAndObjectIdArrays
	createComputeShaderProgram(compShaderProgram, "FillCellIdAndObjectIdArrays.comp", vertShaderParams);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Particles2dCollisionEffect::draw(GLdouble deltaTime)
{
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	GLfloat dt = ((GLfloat)deltaTime) * 1000 * runtimeParams.TimeScale;

	glUseProgram(compShaderProgram);

	glUniform2f(0, windowWidth, windowHeight);
	glUniform1f(1, runtimeParams.ForceScale);
	glUniform1f(2, runtimeParams.VelocityDamping);
	glUniform1f(3, runtimeParams.MinDistanceToAttractor);
	glUniform1f(4, dt);
	glUniform1i(5, isPaused && !isAdvanceOneFrame);
	glUniform1f(6, runtimeParams.particleSize);
	runtimeParams.cellSize = runtimeParams.particleSize;
	glUniform1ui(7, runtimeParams.cellSize);

	GLuint groupCount = ceil(currentParticlesCount / 64.0);
	glDispatchCompute(groupCount, 1, 1);

	//glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCellId);
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellId);
	GLushort r[1000] = { 0 };
	GLushort* ptr = (GLushort*)glMapNamedBufferRange(ssboCellId, 0, currentParticlesCount * 4 * sizeof(GLushort), GL_MAP_READ_BIT);
	GLenum e = glGetError();
	memcpy(r, ptr, sizeof(GLushort) * 1000);
	bool result = glUnmapNamedBuffer(ssboCellId);

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

void Particles2dCollisionEffect::createComputeShaderProgram(GLuint &compShaderProgram, const char* shaderFilePath, std::vector<ShaderParams> shaderParams)
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
