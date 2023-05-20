#include "Particles2dNBodyGravityEffect.h"

#include <string>

Particles2dNBodyGravityEffect::Particles2dNBodyGravityEffect(GLFWwindow* window)
	:AbstractEffect(window)
{
	vertexShaderFilePath = "Particles2dNBodyGravityEffect.vert";
	fragmentShaderFilePath = "Particles2dNBodyGravityEffect.frag";

	startupParams = {};
	startupParams.ParticlesCount = 20000;

	runtimeParams = {};
	runtimeParams.ForceScale = 1.0;
	runtimeParams.VelocityDamping = 0.999;
	runtimeParams.MinDistanceToAttractor = 50.0;
	runtimeParams.TimeScale = 1.0;
	runtimeParams.Color[0] = 0.196;
	runtimeParams.Color[1] = 0.05;
	runtimeParams.Color[2] = 0.941;
	runtimeParams.Color[3] = 0.9;
}

Particles2dNBodyGravityEffect::~Particles2dNBodyGravityEffect()
{
	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &ssbo);
	glDeleteVertexArrays(1, &vao);
}


void Particles2dNBodyGravityEffect::initialize()
{
	for (int i = 0; i < startupParams.ParticlesCount; i++)
	{
		// positions
		particlesData.push_back(random(0.0, windowWidth));
		particlesData.push_back(random(0.0, windowHeight));
		// velocities
		particlesData.push_back(0.0);
		particlesData.push_back(0.0);
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, particlesData.size() * sizeof(GLfloat), &particlesData[0], GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	particlesData.clear();
	currentParticlesCount = startupParams.ParticlesCount;

	std::vector<ShaderParam> vertShaderParams
	{
		ShaderParam {"#define particlesCount 0", "#define particlesCount " + std::to_string(currentParticlesCount)}
	};
	GLint newShaderProgram = createShaderProgramFromFiles(vertShaderParams);
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}

	shaderProgram = newShaderProgram;
	glUseProgram(shaderProgram);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

void Particles2dNBodyGravityEffect::draw(GLdouble deltaTime)
{
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	// vertex shader params
	glUniform2f(0, windowWidth, windowHeight);
	glUniform1f(1, runtimeParams.ForceScale);
	glUniform1f(2, runtimeParams.VelocityDamping);
	glUniform1f(3, runtimeParams.MinDistanceToAttractor);
	glUniform1f(4, runtimeParams.TimeScale);

	// fragment shader params
	glUniform4f(20, runtimeParams.Color[0], runtimeParams.Color[1], runtimeParams.Color[2], runtimeParams.Color[3]);

	glDrawArrays(GL_POINTS, 0, currentParticlesCount);
}

void Particles2dNBodyGravityEffect::drawGUI()
{
	ImGui::Begin("Startup params (N-body)");
	ImGui::InputInt("Particles count", &startupParams.ParticlesCount, 1000, 10000);
	ImGui::End();

	ImGui::Begin("Runtime params (N-body)");
	ImGui::SliderFloat("Force scale", &runtimeParams.ForceScale, -1.0, 10.0);
	ImGui::SliderFloat("Velocity damping", &runtimeParams.VelocityDamping, 0.9, 1.0);
	ImGui::SliderFloat("Min distance", &runtimeParams.MinDistanceToAttractor, 0.01, 100.0);
	ImGui::SliderFloat("Time scale", &runtimeParams.TimeScale, -5.0, 5.0);
	ImGui::ColorPicker4("", runtimeParams.Color, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoLabel);
	ImGui::End();
}

void Particles2dNBodyGravityEffect::restart()
{
	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &ssbo);
	glDeleteVertexArrays(1, &vao);

	initialize();
}


void Particles2dNBodyGravityEffect::keyCallback(int key, int scancode, int action, int mode)
{

}