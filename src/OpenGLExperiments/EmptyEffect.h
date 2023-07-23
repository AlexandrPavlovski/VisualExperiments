#pragma once

#include "AbstractEffect.h"

#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class EmptyEffect : public AbstractEffect
{
public:
	EmptyEffect(GLFWwindow* window);

	virtual ~EmptyEffect();
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

	GLuint vao = 0, ssbo = 0;
};
#pragma once
