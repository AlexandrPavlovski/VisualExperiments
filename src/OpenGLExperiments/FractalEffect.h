#pragma once

#include "AbstractEffect.h"

#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>


class FractalEffect :public AbstractEffect
{
public:
	FractalEffect(GLFWwindow* window);
	virtual ~FractalEffect();

	virtual void initialize();
	virtual void draw(GLdouble deltaTime);
	virtual void drawGUI();
	virtual void restart();

	virtual void keyCallback(int key, int scancode, int action, int mode);

private:
	std::vector<GLfloat> trianglesData;
	GLuint vao = 0, ssbo = 0;
};
