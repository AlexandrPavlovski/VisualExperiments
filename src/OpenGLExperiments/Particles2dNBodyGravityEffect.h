#pragma once

#include "AbstractEffect.h"

#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>


class Particles2dNBodyGravityEffect :public AbstractEffect
{
public:
	Particles2dNBodyGravityEffect(GLFWwindow* window);
	virtual ~Particles2dNBodyGravityEffect();

	virtual void initialize();
	virtual void draw(GLdouble deltaTime);
	virtual void drawGUI();
	virtual void restart();

	virtual void keyCallback(int key, int scancode, int action, int mode);

private:
	struct StartupParams
	{
		int ParticlesCount;
	};

	struct RuntimeParams
	{
		GLfloat ForceScale;
		GLfloat VelocityDamping;
		GLfloat MinDistanceToAttractor;
		GLfloat TimeScale;
		GLfloat Color[4];
	};

	StartupParams startupParams;
	RuntimeParams runtimeParams;

	int currentParticlesCount = 0;
	std::vector<GLfloat> particlesData;
	GLuint vao = 0, ssbo = 0;
};



