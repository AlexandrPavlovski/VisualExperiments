#pragma once

#include "AbstractEffect.h"

#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class EmptyEffect : public AbstractEffect
{
public:
	EmptyEffect(GLFWwindow* window)
		:AbstractEffect(window)
	{
		vertexShaderFileName = "EmptyEffect.vert";
		fragmentShaderFileName = "EmptyEffect.frag";

		startupParams = {};

		runtimeParams = {};
	}

	virtual ~EmptyEffect()
	{
		cleanup();
	}

	virtual void initialize()
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 100 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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
		ImGui::Begin("Startup params (Empty Effect)");
		ImGui::Text("Move attractor: A");
		ImGui::PushItemWidth(120);
		//ImGui::InputInt("Particles count", &startupParams.ParticlesCount, 100000, 1000000);
		ImGui::PopItemWidth();
		ImGui::End();

		ImGui::Begin("Runtime params (Empty Effect)");
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

	GLuint vao = 0, ssbo = 0;
};
