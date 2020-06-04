#pragma once

#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

class AbstractEffect
{
public:
	AbstractEffect(GLFWwindow* window);
	virtual ~AbstractEffect() = 0;

	virtual void initialize() = 0;
	virtual void draw(GLdouble deltaTime) = 0;
	virtual void drawGUI() = 0;
	virtual void restart() = 0;
	void hotReloadShaders();

	virtual void keyCallback(int key, int scancode, int action, int mode);
	virtual void windowSizeCallback(int width, int height);

protected:
	GLFWwindow* window;
	GLint windowWidth, windowHeight;

	const char* vertexShaderFilePath;
	const char* fragmentShaderFilePath;
	GLuint shaderProgram;

	GLfloat random(GLfloat low, GLfloat high);
	int checkShaderCompileErrors(GLuint shader);
	int checkShaderPrgramLinkErrors(GLuint shaderProgram);

	GLuint createShaderProgramFromFiles();
	std::string readShaderFromFile(const char* shaderFilePath);
};
