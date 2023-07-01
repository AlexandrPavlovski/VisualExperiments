#pragma once

#include <string>
#include <vector>

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
	virtual void cleanup() = 0;
	void hotReloadShaders();

	virtual void keyCallback(int key, int scancode, int action, int mode);
	virtual void mouseButtonCallback(int button, int action, int mods);
	virtual void scrollCallback(double xoffset, double yoffset);
	virtual void windowSizeCallback(int width, int height);

	bool isPaused = false;
	bool isAdvanceOneFrame = false;

#ifdef _DEBUG
	const char* shadersFolder = "";
#else
	const char* shadersFolder = "shaders/";
#endif

protected:
	struct ShaderParam
	{
		std::string Placeholder;
		std::string Value;
	};

	GLFWwindow* window;
	GLint windowWidth, windowHeight;

	const char* vertexShaderFileName;
	const char* fragmentShaderFileName;
	GLuint shaderProgram;

	GLfloat random(GLfloat low, GLfloat high);

	GLint createShaderProgramFromFiles(
		std::vector<ShaderParam> vertShaderParams = std::vector<ShaderParam>(),
		std::vector<ShaderParam> fragShaderParams = std::vector<ShaderParam>());
	GLint createShaderProgramFromFiles(
		const char* customVertexShaderFileName,
		const char* customFragmentShaderFileName,
		std::vector<ShaderParam> vertShaderParams = std::vector<ShaderParam>(),
		std::vector<ShaderParam> fragShaderParams = std::vector<ShaderParam>());

	GLint createShader(GLint shaderProgram, GLint shaderType, std::vector<ShaderParam> shaderParams = std::vector<ShaderParam>());
	GLint createShader(const char* shaderFileName, GLint shaderProgram, GLint shaderType, std::vector<ShaderParam> shaderParams = std::vector<ShaderParam>());
	void createComputeShaderProgram(GLuint& compShaderProgram, const char* shaderFileName, std::vector<ShaderParam> shaderParams);
	GLint checkShaderPrgramLinkErrors(GLuint shaderProgram, const char* shaderFileName);


private:
	GLint checkShaderCompileErrors(GLuint shader);
	std::string readShaderFromFile(const char* shaderFileName);
	char* concat(const char* s1, const char* s2);
};
