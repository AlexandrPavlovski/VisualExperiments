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

	struct Particle
	{
		GLfloat PosX;
		GLfloat PosY;
		GLfloat VelX;
		GLfloat VelY;
		GLfloat AccX;
		GLfloat AccY;
		GLfloat Pressure;
		GLfloat Unused;
	};

	StartupParams startupParams;
	RuntimeParams runtimeParams;

	GLint currentParticlesCount = 0;
	GLint currentCellsCount = 0;
	bool isPaused = false;
	bool isAdvanceOneFrame = false;

	std::vector<Particle> particles;
	GLuint vao = 0, ssboParticles = 0, ssboObjectId = 0, ssboCellId = 0, ssboGlobalCounters = 0;
	GLuint fillCellIdAndObjectIdArraysCompShaderProgram = 0,
		radixPhase1Pass1CompShaderProgram = 0, radixPhase1Pass2CompShaderProgram = 0,
		radixPhase2CompShaderProgram = 0,
		radixPhase3Pass1CompShaderProgram = 0, radixPhase3Pass2CompShaderProgram = 0,
		findAndResolveCollisionsCompShaderProgram = 0;

	GLuint maxWorkGroupCount = 0;
	GLuint threadGroupsInWorkGroup = 0;
	GLuint threadsInThreadGroup = 0;
	GLfloat threadsInWorkGroup = 0;
	GLuint elementsPerThread = 0;
	GLuint elementsPerGroup = 0;
	GLuint threadGroupsTotal = 0;

	bool isManualAttractorControlEnabled = false;
	GLuint phase1GroupCount = 0;

	
	void createComputeShaderProgram(GLuint& compShaderProgram, const char* shaderFilePath, std::vector<ShaderParams> shaderParams = std::vector<ShaderParams>());

	template< typename T >
	T* readFromBuffer(int elemCount, GLuint ssbo);

	void cleanup();

	// temp vars
	GLuint buffer = 0;
	GLuint buffer5 = 0;
	GLuint buffer6 = 0;
	GLuint buffer7 = 0;
	GLuint buffer8 = 0;
	GLuint bufferTest = 0;
	GLuint frame = 0;
	
};
