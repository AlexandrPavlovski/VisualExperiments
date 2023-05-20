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
	void hotReloadShaders();

	virtual void keyCallback(int key, int scancode, int action, int mode);
	virtual void mouseButtonCallback(int button, int action, int mods);
	virtual void scrollCallback(double xoffset, double yoffset);
	virtual void windowSizeCallback(int width, int height);

protected:
	struct ShaderParam
	{
		std::string Placeholder;
		std::string Value;
	};

	GLFWwindow* window;
	GLint windowWidth, windowHeight;

	const char* computeShaderFilePath;
	const char* vertexShaderFilePath;
	const char* fragmentShaderFilePath;
	GLuint shaderProgram;

	GLfloat random(GLfloat low, GLfloat high);

	GLint createShaderProgramFromFiles(
		std::vector<ShaderParam> vertShaderParams = std::vector<ShaderParam>(),
		std::vector<ShaderParam> fragShaderParams = std::vector<ShaderParam>());
	GLint createShaderProgramFromFiles(
		const char* customVertexShaderFilePath,
		const char* customFragmentShaderFilePath,
		std::vector<ShaderParam> vertShaderParams = std::vector<ShaderParam>(),
		std::vector<ShaderParam> fragShaderParams = std::vector<ShaderParam>());

	GLint createShader(GLint shaderProgram, GLint shaderType, std::vector<ShaderParam> shaderParams = std::vector<ShaderParam>());
	GLint createShader(const char* shaderFilePath, GLint shaderProgram, GLint shaderType, std::vector<ShaderParam> shaderParams = std::vector<ShaderParam>());
	void createComputeShaderProgram(GLuint& compShaderProgram, const char* shaderFilePath, std::vector<ShaderParam> shaderParams);
	GLint checkShaderPrgramLinkErrors(GLuint shaderProgram);


private:
	GLint checkShaderCompileErrors(GLuint shader);

	std::string readShaderFromFile(const char* shaderFilePath);
};
