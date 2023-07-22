#pragma once

#include "AbstractEffect.h"

#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class EulerianFluidEffect : public AbstractEffect
{
public:
	EulerianFluidEffect(GLFWwindow* window);
	virtual ~EulerianFluidEffect();

	virtual void initialize();
	virtual void draw(GLdouble deltaTime);
	virtual void drawGUI();
	virtual void restart();
	virtual void cleanup();

private:
	struct StartupParams
	{
	};

	struct RuntimeParams
	{
	};

	StartupParams startupParams;
	RuntimeParams runtimeParams;

	GLuint vao = 0, ssboU = 0, ssboV = 0, textureU = 0, textureV = 0, sampler = 0;
	GLuint solveIncompressibilityCompShaderProgram = 0;

	static const GLuint simulationAreaWidth = 1280; // decoupled from screen size
	static const GLuint simulationAreaHeight = 800;
	static const GLuint cellsCount = simulationAreaWidth * simulationAreaHeight;

};
