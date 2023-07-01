#include "Particles2dGravityEffect.h"


Particles2dGravityEffect::Particles2dGravityEffect(GLFWwindow* window)
	:AbstractEffect(window) 
{
	vertexShaderFileName = "Particles2dGravityEffect.vert";
	fragmentShaderFileName = "Particles2dGravityEffect.frag";

	startupParams = {};
	startupParams.ParticlesCount = 1000000;

	runtimeParams = {};
	runtimeParams.AttractorX = windowWidth / 2;
	runtimeParams.AttractorY = windowHeight / 2;
	runtimeParams.ForceScale = 1.0;
	runtimeParams.VelocityDamping = 0.999;
	runtimeParams.MinDistanceToAttractor = 500.0;
	runtimeParams.TimeScale = 1.0;
	runtimeParams.Color[0] = 0.196;
	runtimeParams.Color[1] = 0.05;
	runtimeParams.Color[2] = 0.941;
	runtimeParams.Color[3] = 0.2;
}

Particles2dGravityEffect::~Particles2dGravityEffect()
{
	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &ssbo);
	glDeleteVertexArrays(1, &vao);
}


void Particles2dGravityEffect::initialize()
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

	GLint newShaderProgram = createShaderProgramFromFiles();
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}

	shaderProgram = newShaderProgram;
	glUseProgram(shaderProgram);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

void Particles2dGravityEffect::draw(GLdouble deltaTime)
{
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	if (isManualAttractorControlEnabled)
	{
		glfwGetCursorPos(window, &runtimeParams.AttractorX, &runtimeParams.AttractorY);
	}
	
	// vertex shader params
	glUniform2f(0, runtimeParams.AttractorX, runtimeParams.AttractorY);
	glUniform2f(1, windowWidth, windowHeight);
	glUniform1f(2, runtimeParams.ForceScale);
	glUniform1f(3, runtimeParams.VelocityDamping);
	glUniform1f(4, runtimeParams.MinDistanceToAttractor);
	glUniform1f(5, runtimeParams.TimeScale);

	// fragment shader params
	glUniform4f(20, runtimeParams.Color[0], runtimeParams.Color[1], runtimeParams.Color[2], runtimeParams.Color[3]);

	glDrawArrays(GL_POINTS, 0, currentParticlesCount);
}

void Particles2dGravityEffect::drawGUI()
{
	ImGui::Begin("Startup params (One attractor)");
	ImGui::Text("Move attractor: A");
	ImGui::InputInt("Particles count", &startupParams.ParticlesCount, 100000, 1000000);
	ImGui::End();

	ImGui::Begin("Runtime params (One attractor)");
	ImGui::SliderFloat("Force scale", &runtimeParams.ForceScale, -1.0, 10.0);
	ImGui::SliderFloat("Velocity damping", &runtimeParams.VelocityDamping, 0.9, 1.0);
	ImGui::SliderFloat("Min distance", &runtimeParams.MinDistanceToAttractor, 0.0, 1000.0);
	ImGui::SliderFloat("Time scale", &runtimeParams.TimeScale, 0.0, 10.0);
	ImGui::ColorPicker4("", runtimeParams.Color, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoLabel);
	ImGui::End();
}

void Particles2dGravityEffect::restart()
{
	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &ssbo);
	glDeleteVertexArrays(1, &vao);

	initialize();
}

void Particles2dGravityEffect::cleanup()
{

}

void Particles2dGravityEffect::keyCallback(int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_A && action == GLFW_PRESS)
	{
		isManualAttractorControlEnabled = true;
	}
	if (key == GLFW_KEY_A && action == GLFW_RELEASE)
	{
		isManualAttractorControlEnabled = false;
	}
}