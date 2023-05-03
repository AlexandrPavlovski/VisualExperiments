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
	virtual void windowSizeCallback(int width, int height);

private:
	struct StartupParams
	{
		int Iterations;
		int AntiAliasing;
		bool isDoublePrecision;
	};

	struct RuntimeParams
	{
		GLdouble viewPosX, viewPosY, viewZoom;
		GLfloat a1, a2, a3;
	};

	StartupParams startupParams;
	RuntimeParams runtimeParams;

	bool isLeftMouseBtnDown = false;
	GLdouble cursorPosX = 0.0, cursorPosY = 0.0;
	GLdouble zoomSpeed;
	GLuint samplesCount = 0, samplesWidth = 0, samplesHeight = 0;
	GLuint groupsX = 0, groupsY = 0;
	GLuint frameNumber = 0;


	GLuint vao = 0, ssboCompute = 0;
	GLuint computeShaderProgram = 0;

	const char* fragmentShaderFilePathSingle = "Fractal2dSingleEffect.frag";
	const char* fragmentShaderFilePathDouble = "Fractal2dDoubleEffect.frag";

	template< typename T >
	T* readFromBuffer(int elemCount, GLuint ssbo);

	void resetViewBuffer();
};
