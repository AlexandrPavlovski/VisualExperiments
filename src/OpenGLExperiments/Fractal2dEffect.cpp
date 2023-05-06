#include "Fractal2dEffect.h"

Fractal2dEffect::Fractal2dEffect(GLFWwindow* window)
	:AbstractEffect(window)
{
	vertexShaderFilePath = "Fractal2dEffect.vert";
	fragmentShaderFilePath = fragmentShaderFilePathSingle;

	startupParams = {};
	startupParams.Iterations = 1;
	startupParams.AntiAliasing = 2;

	runtimeParams = {};
	runtimeParams.viewZoom = 2.5;
	runtimeParams.viewPosX = -3.0;
	runtimeParams.viewPosY = -runtimeParams.viewZoom / 2;
	runtimeParams.a1 = 0;
	runtimeParams.a2 = 1;
	runtimeParams.a3 = 0;

	isDoublePrecision = false;
	zoomSpeed = 1;
}

Fractal2dEffect::~Fractal2dEffect()
{
	cleanup();
}


void Fractal2dEffect::initialize()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	ShaderParams iterationsShaderParam   { "#define iterations 0", "#define iterations " + std::to_string(startupParams.Iterations) };
	ShaderParams antiAliasingShaderParam { "#define AA 0",         "#define AA "         + std::to_string(startupParams.AntiAliasing) };

	std::vector<ShaderParams> fragShaderParams
	{
		iterationsShaderParam,
		antiAliasingShaderParam
	};
	fragmentShaderFilePath = fragmentShaderFilePathSingle;
	GLint newShaderProgram = createShaderProgramFromFiles(std::vector<ShaderParams>(), fragShaderParams);
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}
	shaderProgram = newShaderProgram;

	fragmentShaderFilePath = fragmentShaderFilePathDouble;
	newShaderProgram = createShaderProgramFromFiles(std::vector<ShaderParams>(), fragShaderParams);
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}
	shaderProgramDouble = newShaderProgram;

	std::vector<ShaderParams> computeShaderParams
	{
		iterationsShaderParam,
		antiAliasingShaderParam
	};
	createComputeShaderProgram(computeShaderProgramSingle, computeShaderFilePathSingle, computeShaderParams);
	createComputeShaderProgram(computeShaderProgramDouble, computeShaderFilePathDouble, computeShaderParams);

	samplesWidth = windowWidth * startupParams.AntiAliasing;
	samplesHeight = windowHeight * startupParams.AntiAliasing;
	samplesCount = samplesWidth * samplesHeight;

	groupsX = ceil(samplesWidth / 32.0);
	groupsY = ceil(samplesHeight / 32.0);

	createViewBuffer();

	frameNumber = 1;
}

void Fractal2dEffect::draw(GLdouble deltaTime)
{
	GLdouble oldCursorPosX = cursorPosX, oldCursorPosY = cursorPosY;
	glfwGetCursorPos(window, &cursorPosX, &cursorPosY);

	GLdouble zoom = windowHeight / runtimeParams.viewZoom;

	if (isLeftMouseBtnDown)
	{
		GLdouble deltaX = oldCursorPosX - cursorPosX;
		GLdouble deltaY = cursorPosY - oldCursorPosY;

		runtimeParams.viewPosX += deltaX / zoom;
		runtimeParams.viewPosY += deltaY / zoom;

		if (deltaX != 0 || deltaY != 0)
		{
			resetViewBuffer();
		}
	}

	if (frameNumber < 10000) // preventing overflow
	{
		glUseProgram(isDoublePrecision ? computeShaderProgramDouble : computeShaderProgramSingle);
		glUniform1ui(0, samplesWidth);
		glUniform2d(1, runtimeParams.viewPosX, runtimeParams.viewPosY);
		glUniform1d(2, zoom);

		glDispatchCompute(groupsX, groupsY, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		frameNumber++;
	}

	glUseProgram(isDoublePrecision ? shaderProgramDouble : shaderProgram);
	glUniform1ui(0, samplesWidth);
	glUniform1ui(1, frameNumber - 1);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Fractal2dEffect::drawGUI()
{
	ImGui::Begin("Startup params (Fractals)");
	ImGui::Text("Pan: left mouse button");
	ImGui::Text("Zoom: scroll");
	ImGui::Text("Fast zoom: left shift + scroll");
	ImGui::InputInt("Iterations per frame", &startupParams.Iterations, 1, 100);
	ImGui::InputInt("Anti aliasing", &startupParams.AntiAliasing, 1, 1);
	ImGui::End();
}

void Fractal2dEffect::restart()
{
	cleanup();
	initialize();
}

void Fractal2dEffect::keyCallback(int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS)
	{
		zoomSpeed = 10;
	}
	if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE)
	{
		zoomSpeed = 1;
	}
}

void Fractal2dEffect::mouseButtonCallback(int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		isLeftMouseBtnDown = true;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		isLeftMouseBtnDown = false;
	}
}

void Fractal2dEffect::windowSizeCallback(int width, int height)
{
	AbstractEffect::windowSizeCallback(width, height);

	samplesWidth = windowWidth * startupParams.AntiAliasing;
	samplesHeight = windowHeight * startupParams.AntiAliasing;
	samplesCount = samplesWidth * samplesHeight;

	groupsX = ceil(samplesWidth / 32.0);
	groupsY = ceil(samplesHeight / 32.0);

	createViewBuffer();

	frameNumber = 1;
}

void Fractal2dEffect::scrollCallback(double xoffset, double yoffset)
{
	// this is needed because in shader (0, 0) is bottom left with positive direction up
	// while in GLFW (0, 0) is top left with positive direction down
	GLdouble transfomedCursorPosY = windowHeight - cursorPosY;

	GLdouble zoom = windowHeight / runtimeParams.viewZoom;
	GLdouble beforeZoomCursorPosX = cursorPosX / zoom + runtimeParams.viewPosX;
	GLdouble	beforeZoomCursorPosY = transfomedCursorPosY / zoom + runtimeParams.viewPosY;

	GLdouble multiplier = 1 + zoomSpeed / 10;
	if (yoffset < 0)
	{
		runtimeParams.viewZoom *= multiplier;
	}
	else
	{
		runtimeParams.viewZoom /= multiplier;
	}

	zoom = windowHeight / runtimeParams.viewZoom;
	GLdouble afterZoomCursorPosX = cursorPosX / zoom + runtimeParams.viewPosX;
	GLdouble	afterZoomCursorPosY = transfomedCursorPosY / zoom + runtimeParams.viewPosY;

	runtimeParams.viewPosX += beforeZoomCursorPosX - afterZoomCursorPosX;
	runtimeParams.viewPosY += beforeZoomCursorPosY - afterZoomCursorPosY;

	bool isPrecisionChanged = isDoublePrecision;
	isDoublePrecision = zoom > 16739649;
	isPrecisionChanged = isDoublePrecision != isPrecisionChanged;

	if (isPrecisionChanged)
	{
		createViewBuffer();
	}
	else
	{
		resetViewBuffer();
	}
}

template< typename T >
T* Fractal2dEffect::readFromBuffer(int elemCount, GLuint ssbo)
{
	T* buf = new T[elemCount];
	T* p = (T*)glMapNamedBufferRange(ssbo, 0, elemCount * sizeof(T), GL_MAP_READ_BIT);
	memcpy(buf, p, elemCount * sizeof(T));
	glUnmapNamedBuffer(ssbo);

	return buf;
}

void Fractal2dEffect::createViewBuffer()
{
	glGenBuffers(1, &ssboCompute);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboCompute);
	if (isDoublePrecision)
	{
		glBufferData(GL_SHADER_STORAGE_BUFFER, samplesCount * sizeof(GLdouble) * 3, NULL, GL_DYNAMIC_DRAW);
	}
	else
	{
		glBufferData(GL_SHADER_STORAGE_BUFFER, samplesCount * sizeof(GLfloat) * 3, NULL, GL_DYNAMIC_DRAW);
	}
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, NULL);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Fractal2dEffect::resetViewBuffer()
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboCompute);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, NULL);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	frameNumber = 1;
}

void Fractal2dEffect::cleanup()
{
	glDeleteProgram(shaderProgram);
	glDeleteProgram(shaderProgramDouble);
	glDeleteProgram(computeShaderProgramSingle);
	glDeleteProgram(computeShaderProgramDouble);
	glDeleteBuffers(1, &ssboCompute);
}
