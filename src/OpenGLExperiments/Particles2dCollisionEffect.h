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

	GLint currentParticlesCount = 0;
	bool isPaused = false;
	bool isAdvanceOneFrame = false;

	std::vector<GLfloat> particlesData;
	GLuint vao = 0, ssbo = 0, ssboObjectId = 0, ssboCellId = 0, ssboGlobalCounters = 0;
	GLuint fillCellIdAndObjectIdArraysCompShaderProgram = 0,
		radixPhase1Pass1CompShaderProgram = 0, radixPhase1Pass2CompShaderProgram = 0,
		radixPhase2CompShaderProgram = 0,
		radixPhase3Pass1CompShaderProgram = 0, radixPhase3Pass2CompShaderProgram = 0;

	GLuint maxWorkGroupCount = 0;
	GLuint threadGroupsInWorkGroup = 0;
	GLuint threadsInThreadGroup = 0;
	GLfloat threadsInWorkGroup = 0;
	GLuint cellIdsLength = 0;
	GLuint elementsPerThread = 0;
	GLuint elementsPerGroup = 0;
	GLuint threadGroupsTotal = 0;

	bool isManualAttractorControlEnabled = false;
	GLuint phase1GroupCount = 0;

	void createComputeShaderProgram(GLuint& compShaderProgram, const char* shaderFilePath, std::vector<ShaderParams> shaderParams = std::vector<ShaderParams>());

	// temp vars
	GLuint buffer = 0;
	GLuint buffer2 = 0;
	GLuint buffer3 = 0;
	GLuint buffer7 = 0;
	GLuint frame = 0;
	
};
