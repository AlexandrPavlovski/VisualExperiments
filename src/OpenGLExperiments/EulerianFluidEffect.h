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
		GLint substeps;
	};

	StartupParams startupParams;
	RuntimeParams runtimeParams;

	GLuint vao = 0, ssboCellType = 0, textureU0 = 0, textureV0 = 0, textureU1 = 0, textureV1 = 0, sampler = 0;
	GLuint oddShaderProgram = 0,
		solveIncompressibilityOddCompShaderProgram = 0,
		solveIncompressibilityEvenCompShaderProgram = 0,
		advectVelocitiesOddCompShaderProgram = 0,
		advectVelocitiesEvenCompShaderProgram = 0;
	GLuint bufferTest = 0;

	static const GLuint simulationAreaWidth = 1280; // decoupled from screen size
	static const GLuint simulationAreaHeight = 800;
	static const GLuint cellsCount = simulationAreaWidth * simulationAreaHeight;
	GLfloat cellsPerWorkGroup = 64;
	GLuint workGroupsCountX = ceil(simulationAreaWidth / cellsPerWorkGroup);
	GLuint workGroupsCountY = ceil(simulationAreaHeight / cellsPerWorkGroup);

	bool isOddFrame = true;

	void CreateTextureField(GLuint* texture, GLenum textureEnum, GLsizei width, GLsizei height, const void* data, GLuint unit);
	template< typename T >
	T* readFromBuffer(int elemCount, GLuint ssbo);
};
