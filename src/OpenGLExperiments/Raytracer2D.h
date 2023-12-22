#pragma once

#include "AbstractEffect.h"

#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Raytracer2D : public AbstractEffect
{
public:
	Raytracer2D(GLFWwindow* window);

	virtual ~Raytracer2D();
	virtual void initialize();
	virtual void draw(GLdouble deltaTime);
	virtual void drawGUI();
	virtual void restart();
	virtual void cleanup();

private:
	struct StartupParams
	{
		GLint simulationAreaWidth;
		GLint simulationAreaHeight;
	};

	struct RuntimeParams
	{
	};

	StartupParams startupParams;
	RuntimeParams runtimeParams;

	GLuint vao = 0, ssbo = 0;

	GLuint floatTextureShaderProgram = 0;
	GLuint floatTexture = 0, texture1 = 0;

	GLuint floatFramebuffer = 0;

	static const GLuint linesPerFrameCount = 50000;
	GLfloat linesForFrame[linesPerFrameCount * 4];

	GLuint frameCount = 0;
};
