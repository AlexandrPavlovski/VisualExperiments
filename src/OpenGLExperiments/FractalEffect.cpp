#include "FractalEffect.h"

FractalEffect::FractalEffect(GLFWwindow* window)
	:AbstractEffect(window)
{
	vertexShaderFilePath = "FractalEffect.vert";
	fragmentShaderFilePath = "FractalEffect.frag";
}

FractalEffect::~FractalEffect()
{
	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &ssbo);
	glDeleteVertexArrays(1, &vao);
}


void FractalEffect::initialize()
{
	// triangle strip
	trianglesData.push_back(-1.0);trianglesData.push_back(1.0);
	trianglesData.push_back(1.0);trianglesData.push_back(1.0);
	trianglesData.push_back(-1.0);trianglesData.push_back(-1.0);
	trianglesData.push_back(1.0);trianglesData.push_back(-1.0);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, trianglesData.size() * sizeof(GLfloat), &trianglesData[0], GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	GLuint newShaderProgram = createShaderProgramFromFiles();
	if (newShaderProgram == 0)
	{
		throw "Initialize failed";
	}

	shaderProgram = newShaderProgram;
	glUseProgram(shaderProgram);
}

void FractalEffect::draw(GLdouble deltaTime)
{
	// vertex shader params
	//glUniform2f(0, runtimeParams.AttractorX, runtimeParams.AttractorY);

	// fragment shader params
	//glUniform4f(20, runtimeParams.Color[0], runtimeParams.Color[1], runtimeParams.Color[2], runtimeParams.Color[3]);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void FractalEffect::drawGUI()
{
	//ImGui::Begin("Startup params");
	//ImGui::Text("Move attractor: A");
	//ImGui::InputInt("Particles count", &startupParams.ParticlesCount, 100000, 1000000);
	//ImGui::End();
	//
	//ImGui::Begin("Runtime params");
	//ImGui::SliderFloat("Force scale", &runtimeParams.ForceScale, -1.0, 10.0);
	//ImGui::SliderFloat("Velocity damping", &runtimeParams.VelocityDamping, 0.9, 1.0);
	//ImGui::SliderFloat("Min distance", &runtimeParams.MinDistanceToAttractor, 0.0, 1000.0);
	//ImGui::SliderFloat("Time scale", &runtimeParams.TimeScale, 0.0, 10.0);
	//ImGui::ColorPicker4("", runtimeParams.Color, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoLabel);
	//ImGui::End();
}

void FractalEffect::restart()
{
	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &ssbo);
	glDeleteVertexArrays(1, &vao);

	initialize();
}


void FractalEffect::keyCallback(int key, int scancode, int action, int mode)
{
	//if (key == GLFW_KEY_A && action == GLFW_PRESS)
	//{
	//	isManualAttractorControlEnabled = true;
	//}
	//if (key == GLFW_KEY_A && action == GLFW_RELEASE)
	//{
	//	isManualAttractorControlEnabled = false;
	//}
}