#include "AbstractEffect.h"

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

AbstractEffect::AbstractEffect(GLFWwindow* window)
{
	this->window = window;
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
	shaderProgram = 0;
	vertexShaderFilePath = NULL;
	fragmentShaderFilePath = NULL;
}

AbstractEffect::~AbstractEffect() {}


GLfloat AbstractEffect::random(GLfloat low, GLfloat high)
{
	return low + static_cast <GLfloat> (rand()) / (static_cast <GLfloat> (RAND_MAX / (high - low)));
}

int AbstractEffect::checkShaderCompileErrors(GLuint shader)
{
	GLint isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

		std::cout << "SHADER COMPILATION FAILED" << std::endl;
		for (auto i : errorLog)
		{
			std::cout << i;
		}
		std::cout << std::endl;

		return -1;
	}

	return 0;
}

int AbstractEffect::checkShaderPrgramLinkErrors(GLuint shaderProgram)
{
	GLint isLinked = 0;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isLinked);
	if (isLinked == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &maxLength);

		std::vector<GLchar> errorLog(maxLength);
		glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, &errorLog[0]);

		std::cout << "SHADER PROGRAM LINKING FAILED" << std::endl;
		for (auto i : errorLog)
		{
			std::cout << i;
		}
		std::cout << std::endl;

		return -1;
	}

	return 0;
}

void AbstractEffect::keyCallback(int key, int scancode, int action, int mode)
{
	
}

void AbstractEffect::mouseButtonCallback(int button, int action, int mods)
{

}

void AbstractEffect::scrollCallback(double xoffset, double yoffset)
{

}

void AbstractEffect::windowSizeCallback(int width, int height)
{
	windowWidth = width;
	windowHeight = height;
}

void AbstractEffect::hotReloadShaders()
{
	GLuint newShaderProgram = createShaderProgramFromFiles();
	if (newShaderProgram != 0)
	{
		glDeleteProgram(shaderProgram);
		shaderProgram = newShaderProgram;
		glUseProgram(shaderProgram);
	}
}

GLuint AbstractEffect::createShaderProgramFromFiles(std::vector<ShaderParams> vertShaderParams, std::vector<ShaderParams> fragShaderParams)
{
	std::string vertexShaderStr = readShaderFromFile(vertexShaderFilePath);
	std::string fragmentShaderStr = readShaderFromFile(fragmentShaderFilePath);

	for (auto p : vertShaderParams)
	{
		vertexShaderStr = std::regex_replace(vertexShaderStr, std::regex(p.Placeholder), p.Value);
	}
	for (auto p : fragShaderParams)
	{
		fragmentShaderStr = std::regex_replace(fragmentShaderStr, std::regex(p.Placeholder), p.Value);
	}

	GLuint newShaderProgram = glCreateProgram();
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	
	const char* shaderStr = vertexShaderStr.c_str();
	glShaderSource(vertexShader, 1, &shaderStr, NULL);
	glCompileShader(vertexShader);
	int result = checkShaderCompileErrors(vertexShader);
	if (result == -1)
	{
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		glDeleteProgram(newShaderProgram);
		return 0;
	}

	shaderStr = fragmentShaderStr.c_str();
	glShaderSource(fragmentShader, 1, &shaderStr, NULL);
	glCompileShader(fragmentShader);
	result = checkShaderCompileErrors(fragmentShader);
	if (result == -1)
	{
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		glDeleteProgram(newShaderProgram);
		return 0;
	}

	glAttachShader(newShaderProgram, vertexShader);
	glAttachShader(newShaderProgram, fragmentShader);

	glLinkProgram(newShaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	result = checkShaderPrgramLinkErrors(newShaderProgram);
	if (result == -1)
	{
		glDeleteProgram(newShaderProgram);
		return 0;
	}

	return newShaderProgram;
}

std::string AbstractEffect::readShaderFromFile(const char* shaderFilePath)
{
	std::ifstream ifs(shaderFilePath);
	std::stringstream buffer;
	buffer << ifs.rdbuf();
	return buffer.str();
}