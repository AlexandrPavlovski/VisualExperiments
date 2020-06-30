#pragma once

#include "AbstractEffect.h"

#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>


class Fractal2dEffect :public AbstractEffect
{
public:
	Fractal2dEffect(GLFWwindow* window);
	virtual ~Fractal2dEffect();

	virtual void initialize();
	virtual void draw(GLdouble deltaTime);
	virtual void drawGUI();
	virtual void restart();

	virtual void keyCallback(int key, int scancode, int action, int mode);
	virtual void mouseButtonCallback(int button, int action, int mods);
	virtual void scrollCallback(double xoffset, double yoffset);

private:
	struct InitialParams
	{
		GLfloat viewPosX;
		GLfloat viewPosY;
		GLfloat viewZoom;
	};
	InitialParams initialParams;

	struct StartupParams
	{
		int Iterations;
	};
	StartupParams startupParams;

	bool isLeftMouseBtnDown = false;
	GLdouble cursorPosX, cursorPosY;
	GLfloat viewPosX, viewPosY, viewZoom, zoomSpeed;

	std::vector<GLfloat> trianglesData;
	GLuint vao = 0, ssbo = 0;
};
