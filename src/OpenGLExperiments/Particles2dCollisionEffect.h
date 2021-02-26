#pragma once

#include "AbstractEffect.h"

#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>


class Particles2dCollisionEffect :public AbstractEffect
{
public:
	Particles2dCollisionEffect(GLFWwindow* window);
	virtual ~Particles2dCollisionEffect();

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
		GLdouble AttractorX;
		GLdouble AttractorY;
		GLfloat ForceScale;
		GLfloat VelocityDamping;
		GLfloat MinDistanceToAttractor;
		GLfloat TimeScale;
		GLfloat Color[4];
		GLfloat particleSize;
		GLuint cellSize;
	};

	StartupParams startupParams;
	RuntimeParams runtimeParams;

	int currentParticlesCount = 0;
	bool isPaused = false;
	bool isAdvanceOneFrame = false;

	std::vector<GLfloat> particlesData;
	GLuint vao = 0, ssbo = 0, ssboObjectId = 0, ssboCellId = 0;

	bool isManualAttractorControlEnabled = false;


	void createComputeShaderProgram(GLuint& compShaderProgram, const char* shaderFilePath, std::vector<ShaderParams> shaderParams = std::vector<ShaderParams>());
};
