#include "EulerianFluidEffect.h"


EulerianFluidEffect::EulerianFluidEffect(GLFWwindow* window)
	:AbstractEffect(window)
{
	vertexShaderFileName = "EulerianFluidEffect.vert";
	fragmentShaderFileName = "EulerianFluidEffect.frag";

	startupParams = {};

	runtimeParams = {};
}

EulerianFluidEffect::~EulerianFluidEffect()
{
	cleanup();
}


void EulerianFluidEffect::initialize()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenTextures(1, &textureU);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureU);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, simulationAreaWidth, simulationAreaHeight, 0, GL_RED, GL_FLOAT, NULL);
	//std::vector<float> data{ 1.0, 0.0, 1.0, 0.0 };
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 2, 2, 0, GL_RED, GL_FLOAT, &data[0]);
	glBindImageTexture(0, textureU, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

	GLint newShaderProgram = createShaderProgramFromFiles();
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}

	shaderProgram = newShaderProgram;

	std::vector<ShaderParam> computeShaderParams
	{
	};
	createComputeShaderProgram(solveIncompressibilityCompShaderProgram, "SolveIncompressibility.comp", computeShaderParams);
}

void EulerianFluidEffect::draw(GLdouble deltaTime)
{
	glUseProgram(solveIncompressibilityCompShaderProgram);
	glDispatchCompute(1, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	//std::vector<float> compute_data(cellsCount);
	//glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, compute_data.data());

	glUseProgram(shaderProgram);
	glUniform1i(0, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void EulerianFluidEffect::drawGUI()
{
	ImGui::Begin("Startup params (Eulerian Fluid)");
	ImGui::Text("Move attractor: A");
	ImGui::PushItemWidth(120);
	//ImGui::InputInt("Particles count", &startupParams.ParticlesCount, 100000, 1000000);
	ImGui::PopItemWidth();
	ImGui::End();

	ImGui::Begin("Runtime params (Eulerian Fluid)");
	//ImGui::SliderFloat("Force scale", &runtimeParams.ForceScale, -1.0, 10.0);
	//ImGui::SliderFloat("Velocity damping", &runtimeParams.VelocityDamping, 0.9, 1.0);
	//ImGui::SliderFloat("Min distance", &runtimeParams.MinDistanceToAttractor, 0.0, 1000.0);
	//ImGui::SliderFloat("Time scale", &runtimeParams.TimeScale, 0.0, 10.0);
	//ImGui::ColorPicker4("", runtimeParams.Color, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoLabel);
	ImGui::End();
}

void EulerianFluidEffect::restart()
{
	cleanup();
	initialize();
}

void EulerianFluidEffect::cleanup()
{
	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &ssboU);
	glDeleteVertexArrays(1, &vao);
}

void TEMP() {
	int i = 0;
}
