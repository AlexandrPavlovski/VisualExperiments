#include "Particles2dCollisionEffect.h"

Particles2dCollisionEffect::Particles2dCollisionEffect(GLFWwindow* window)
	:AbstractEffect(window)
{
	computeShaderFilePath = "Particles2dCollisionEffect.comp";
	vertexShaderFilePath = "Particles2dCollisionEffect.vert";
	fragmentShaderFilePath = "Particles2dCollisionEffect.frag";

	startupParams = {};
	startupParams.ParticlesCount = 20000;

	runtimeParams = {};
	runtimeParams.ForceScale = 1.0;
	runtimeParams.VelocityDamping = 0.999;
	runtimeParams.MinDistanceToAttractor = 20.0;
	runtimeParams.TimeScale = 0.03;
	runtimeParams.Color[0] = 1.0;
	runtimeParams.Color[1] = 1.0;
	runtimeParams.Color[2] = 1.0;
	runtimeParams.Color[3] = 1.0;
	runtimeParams.Size = 10.0;
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
	for (int i = 0; i < startupParams.ParticlesCount; i++)
	{
		// pressure
		particlesData.push_back(0.0);
		// unused
		particlesData.push_back(0.0);
		particlesData.push_back(0.0);
		particlesData.push_back(0.0);
	}

	//particlesData.push_back(700.0);
	//particlesData.push_back(400.0);
	//particlesData.push_back(5.0);
	//particlesData.push_back(0.0);

	//particlesData.push_back(800.0);
	//particlesData.push_back(400.0);
	//particlesData.push_back(0.0);
	//particlesData.push_back(0.0);

	//particlesData.push_back(0.0);
	//particlesData.push_back(100.0);
	//particlesData.push_back(0.0);
	//particlesData.push_back(0.0);

	//particlesData.push_back(10.0);
	//particlesData.push_back(10.0);
	//particlesData.push_back(0.0);
	//particlesData.push_back(0.0);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, particlesData.size() * sizeof(GLfloat), &particlesData[0], GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	particlesData.clear();
	currentParticlesCount = startupParams.ParticlesCount;

	std::vector<ShaderParams> vertShaderParams
	{
		ShaderParams {"#define particlesCount 0", "#define particlesCount " + std::to_string(currentParticlesCount)}
	};
	GLuint newShaderProgram = createShaderProgramFromFiles(vertShaderParams, vertShaderParams);
	if (newShaderProgram == 0)
	{
		throw "Initialize failed";
	}

	shaderProgram = newShaderProgram;
	glUseProgram(shaderProgram);


	newShaderProgram = glCreateProgram();
	if (createShader(newShaderProgram, GL_COMPUTE_SHADER, vertShaderParams) == 0)
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

void Particles2dCollisionEffect::draw(GLdouble deltaTime)
{
	GLfloat dt = ((GLfloat)deltaTime) * 1000 * runtimeParams.TimeScale;

	glUseProgram(compShaderProgram);

	glUniform2f(0, windowWidth, windowHeight);
	glUniform1f(1, runtimeParams.ForceScale);
	glUniform1f(2, runtimeParams.VelocityDamping);
	glUniform1f(3, runtimeParams.MinDistanceToAttractor);
	glUniform1f(4, dt);
	glUniform1i(5, isPaused && !isAdvanceOneFrame);
	glUniform1f(6, runtimeParams.Size);

	glDispatchCompute(ceil(currentParticlesCount / 64.0), 1, 1);


	glUseProgram(shaderProgram);

	// vertex shader params
	glUniform2f(0, windowWidth, windowHeight);
	glUniform1f(1, runtimeParams.ForceScale);
	glUniform1f(2, runtimeParams.VelocityDamping);
	glUniform1f(3, runtimeParams.MinDistanceToAttractor);
	glUniform1f(4, dt);
	glUniform1i(5, isPaused && !isAdvanceOneFrame);
	glUniform1f(6, runtimeParams.Size);
	// fragment shader params
	glUniform4f(20, runtimeParams.Color[0], runtimeParams.Color[1], runtimeParams.Color[2], runtimeParams.Color[3]);

	glPointSize(runtimeParams.Size);
	glDrawArrays(GL_POINTS, 0, currentParticlesCount);

	isAdvanceOneFrame = false;
}

void Particles2dCollisionEffect::drawGUI()
{
	ImGui::Begin("Startup params (N-body)");
	ImGui::InputInt("Particles count", &startupParams.ParticlesCount, 1000, 10000);
	ImGui::End();

	ImGui::Begin("Runtime params (N-body)");
	ImGui::SliderFloat("Force scale", &runtimeParams.ForceScale, -1.0, 10.0);
	ImGui::SliderFloat("Velocity damping", &runtimeParams.VelocityDamping, 0.9, 1.0);
	ImGui::SliderFloat("Min distance", &runtimeParams.MinDistanceToAttractor, 0.01, 100.0);
	ImGui::SliderFloat("Time scale", &runtimeParams.TimeScale, 0.0, 5.0);
	ImGui::SliderFloat("Size", &runtimeParams.Size, 1.0, 500.0);
	ImGui::ColorPicker4("", runtimeParams.Color, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoLabel);
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
