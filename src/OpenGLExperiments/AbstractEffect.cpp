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
	if (newShaderProgram != -1)
	{
		glDeleteProgram(shaderProgram);
		shaderProgram = newShaderProgram;
		glUseProgram(shaderProgram);
	}
}

GLint AbstractEffect::createShaderProgramFromFiles(std::vector<ShaderParams> vertShaderParams, std::vector<ShaderParams> fragShaderParams)
{
	GLint newShaderProgram = glCreateProgram();

	if (vertexShaderFilePath)
	{
		if (createShader(newShaderProgram, GL_VERTEX_SHADER, vertShaderParams) == -1)
		{
			return -1;
		}
	}
	if (fragmentShaderFilePath)
	{
		if (createShader(newShaderProgram, GL_FRAGMENT_SHADER, fragShaderParams) == -1)
		{
			return -1;
		}
	}

	glLinkProgram(newShaderProgram);

	if (checkShaderPrgramLinkErrors(newShaderProgram) == -1)
	{
		glDeleteProgram(newShaderProgram);
		return -1;
	}

	return newShaderProgram;
}


GLint AbstractEffect::createShader(GLint shaderProgram, GLint shaderType, std::vector<ShaderParams> shaderParams)
{
	const char* shaderFilePath;
	switch (shaderType)
	{
	case GL_COMPUTE_SHADER: shaderFilePath = computeShaderFilePath; break;
	case GL_VERTEX_SHADER: shaderFilePath = vertexShaderFilePath; break;
	case GL_FRAGMENT_SHADER: shaderFilePath = fragmentShaderFilePath; break;
	default:
		throw "Shader type is not supported";
	}

	return createShader(shaderFilePath, shaderProgram, shaderType, shaderParams);
}

GLint AbstractEffect::createShader(const char* shaderFilePath, GLint shaderProgram, GLint shaderType, std::vector<ShaderParams> shaderParams)
{
	std::string shaderStr = readShaderFromFile(shaderFilePath);
	for (auto p : shaderParams)
	{
		shaderStr = std::regex_replace(shaderStr, std::regex(p.Placeholder), p.Value);
	}

	GLuint shader = glCreateShader(shaderType);

	const char* shaderCstr = shaderStr.c_str();
	glShaderSource(shader, 1, &shaderCstr, NULL);
	glCompileShader(shader);

	if (checkShaderCompileErrors(shader) == -1)
	{
		glDeleteProgram(shaderProgram);
		glDeleteShader(shader);
		return -1;
	}

	glAttachShader(shaderProgram, shader);
	glDeleteShader(shader);

	return 1;
}

GLint AbstractEffect::checkShaderCompileErrors(GLuint shader)
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

GLint AbstractEffect::checkShaderPrgramLinkErrors(GLuint shaderProgram)
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

std::string AbstractEffect::readShaderFromFile(const char* shaderFilePath)
{
	std::ifstream ifs(shaderFilePath);
	std::stringstream buffer;
	buffer << ifs.rdbuf();
	return buffer.str();
}