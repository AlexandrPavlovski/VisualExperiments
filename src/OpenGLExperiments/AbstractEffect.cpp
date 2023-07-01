#include "AbstractEffect.h"

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <cstring>

AbstractEffect::AbstractEffect(GLFWwindow* window)
{
	this->window = window;
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
	shaderProgram = 0;
	vertexShaderFileName = NULL;
	fragmentShaderFileName = NULL;
}

AbstractEffect::~AbstractEffect() {}


GLfloat AbstractEffect::random(GLfloat low, GLfloat high)
{
	return low + static_cast <GLfloat> (rand()) / (static_cast <GLfloat> (RAND_MAX / (high - low)));
}

void AbstractEffect::keyCallback(int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		isPaused = !isPaused;
	}

	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS && isPaused)
	{
		isAdvanceOneFrame = true;
	}
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

GLint AbstractEffect::createShaderProgramFromFiles(
	const char* customVertexShaderFileName,
	const char* customFragmentShaderFileName,
	std::vector<ShaderParam> vertShaderParams,
	std::vector<ShaderParam> fragShaderParams)
{
	const char* tempVertexShaderFileName = vertexShaderFileName;
	const char* tempFragmentShaderFileName = fragmentShaderFileName;

	vertexShaderFileName = customVertexShaderFileName;
	fragmentShaderFileName = customFragmentShaderFileName;

	GLint newShaderProgram = createShaderProgramFromFiles(vertShaderParams, fragShaderParams);

	vertexShaderFileName = tempVertexShaderFileName;
	fragmentShaderFileName = tempFragmentShaderFileName;

	return newShaderProgram;
}

GLint AbstractEffect::createShaderProgramFromFiles(std::vector<ShaderParam> vertShaderParams, std::vector<ShaderParam> fragShaderParams)
{
	GLint newShaderProgram = glCreateProgram();

	if (vertexShaderFileName)
	{
		if (createShader(newShaderProgram, GL_VERTEX_SHADER, vertShaderParams) == -1)
		{
			return -1;
		}
	}
	if (fragmentShaderFileName)
	{
		if (createShader(newShaderProgram, GL_FRAGMENT_SHADER, fragShaderParams) == -1)
		{
			return -1;
		}
	}

	glLinkProgram(newShaderProgram);

	if (checkShaderPrgramLinkErrors(newShaderProgram, vertexShaderFileName) == -1)
	{
		glDeleteProgram(newShaderProgram);
		return -1;
	}

	return newShaderProgram;
}


GLint AbstractEffect::createShader(GLint shaderProgram, GLint shaderType, std::vector<ShaderParam> shaderParams)
{
	const char* shaderFileName;
	switch (shaderType)
	{
	case GL_VERTEX_SHADER: shaderFileName = (shadersFolder + vertexShaderFileName).c_str(); break;
	case GL_FRAGMENT_SHADER: shaderFileName = (shadersFolder + fragmentShaderFileName).c_str(); break;
	default:
		throw "Shader type is not supported";
	}

	return createShader(shaderFileName, shaderProgram, shaderType, shaderParams);
}

GLint AbstractEffect::createShader(const char* shaderFileName, GLint shaderProgram, GLint shaderType, std::vector<ShaderParam> shaderParams)
{
	std::string shaderStr = readShaderFromFile(shaderFileName);
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

void AbstractEffect::createComputeShaderProgram(GLuint& compShaderProgram, const char* shaderFileName, std::vector<ShaderParam> shaderParams)
{
	GLint newShaderProgram = glCreateProgram();
	if (createShader(shaderFileName, newShaderProgram, GL_COMPUTE_SHADER, shaderParams) == -1)
	{
		throw "Compute shader compilation failed";
	}
	glLinkProgram(newShaderProgram);
	if (checkShaderPrgramLinkErrors(newShaderProgram, shaderFileName) == -1)
	{
		glDeleteProgram(newShaderProgram);
		throw "Compute shader program linking failed";
	}
	compShaderProgram = newShaderProgram;
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

GLint AbstractEffect::checkShaderPrgramLinkErrors(GLuint shaderProgram, const char* shaderFileName)
{
	GLint isLinked = 0;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isLinked);
	if (isLinked == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &maxLength);

		std::vector<GLchar> errorLog(maxLength);
		glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, &errorLog[0]);

		std::cout << "SHADER PROGRAM LINKING FAILED | " << shaderFileName << std::endl;
		for (auto i : errorLog)
		{
			std::cout << i;
		}
		std::cout << std::endl;

		return -1;
	}

	return 0;
}

std::string AbstractEffect::readShaderFromFile(const char* shaderFileName)
{
	std::ifstream ifs(shadersFolder + shaderFileName);
	std::stringstream buffer;
	buffer << ifs.rdbuf();
	return buffer.str();
}
