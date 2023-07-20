#pragma once

#include "AbstractEffect.h"

#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class EulerianFluidEffect : public AbstractEffect
{
public:
	EulerianFluidEffect(GLFWwindow* window)
		:AbstractEffect(window)
	{
		vertexShaderFileName = "EulerianFluidEffect.vert";
		fragmentShaderFileName = "EulerianFluidEffect.frag";

		startupParams = {};

		runtimeParams = {};
	}

	virtual ~EulerianFluidEffect()
	{
		cleanup();
	}

	virtual void initialize()
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		createSsbo(&ssboU, 0, cellsCount, NULL, GL_DYNAMIC_DRAW);
		createSsbo(&ssboV, 0, cellsCount, NULL, GL_DYNAMIC_DRAW);


		GLint newShaderProgram = createShaderProgramFromFiles();
		if (newShaderProgram == -1)
		{
			throw "Initialize failed";
		}

		shaderProgram = newShaderProgram;
		glUseProgram(shaderProgram);
	}

	virtual void draw(GLdouble deltaTime)
	{
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	virtual void drawGUI()
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

	virtual void restart()
	{
		cleanup();
		initialize();
	}

	virtual void cleanup()
	{
		glDeleteProgram(shaderProgram);
		glDeleteBuffers(1, &ssbo);
		glDeleteVertexArrays(1, &vao);
	}

private:
	struct StartupParams
	{
	};

	struct RuntimeParams
	{
	};

	StartupParams startupParams;
	RuntimeParams runtimeParams;

	GLuint vao = 0, ssboU = 0, ssboV;

	static const GLuint simulationAreaWidth = 1280; // decoupled from screen size
	static const GLuint simulationAreaHeight = 800;
	static const GLuint cellsCount = simulationAreaWidth * simulationAreaHeight;

};
