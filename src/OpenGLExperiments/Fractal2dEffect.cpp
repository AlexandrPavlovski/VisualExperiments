#include "Fractal2dEffect.h"

Fractal2dEffect::Fractal2dEffect(GLFWwindow* window)
	:AbstractEffect(window)
{
	vertexShaderFilePath = "Fractal2dEffect.vert";
	fragmentShaderFilePath = fragmentShaderFilePathSingle;

	startupParams = {};
	startupParams.Iterations = 100;
	startupParams.AntiAliasing = 1;
	startupParams.isDoublePrecision = false;

	runtimeParams = {};
	runtimeParams.viewZoom = 2.5;
	runtimeParams.viewPosX = -3.0;
	runtimeParams.viewPosY = -runtimeParams.viewZoom / 2;
	runtimeParams.a1 = 0;
	runtimeParams.a2 = 1;
	runtimeParams.a3 = 0;

	zoomSpeed = 1;
}

Fractal2dEffect::~Fractal2dEffect()
{
	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &ssbo);
	glDeleteVertexArrays(1, &vao);
}


void Fractal2dEffect::initialize()
{
	if (startupParams.isDoublePrecision)
	{
		isDoublePrecision = true;
		fragmentShaderFilePath = fragmentShaderFilePathDouble;
	}
	else
	{
		isDoublePrecision = false;
		fragmentShaderFilePath = fragmentShaderFilePathSingle;
	}

	std::vector<GLfloat> trianglesData;
	// triangle strip
	trianglesData.push_back(-1.0);trianglesData.push_back(1.0);
	trianglesData.push_back(1.0);trianglesData.push_back(1.0);
	trianglesData.push_back(-1.0);trianglesData.push_back(-1.0);
	trianglesData.push_back(1.0);trianglesData.push_back(-1.0);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, trianglesData.size() * sizeof(GLfloat), &trianglesData[0], GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	std::vector<ShaderParams> fragShaderParams
	{
		ShaderParams {"#define iterations 0", "#define iterations " + std::to_string(startupParams.Iterations)},
		ShaderParams {"#define AA 0", "#define AA " + std::to_string(startupParams.AntiAliasing)}
	};
	GLint newShaderProgram = createShaderProgramFromFiles(std::vector<ShaderParams>(), fragShaderParams);
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}

	shaderProgram = newShaderProgram;
	glUseProgram(shaderProgram);
}

void Fractal2dEffect::draw(GLdouble deltaTime)
{
	GLdouble oldCursorPosX = cursorPosX, oldCursorPosY = cursorPosY;
	glfwGetCursorPos(window, &cursorPosX, &cursorPosY);

	GLdouble zoom = windowHeight / runtimeParams.viewZoom;

	if (isRightMouseBtnDown)
	{
		GLdouble deltaX = oldCursorPosX - cursorPosX;
		GLdouble deltaY = cursorPosY - oldCursorPosY;

		runtimeParams.viewPosX += deltaX / zoom;
		runtimeParams.viewPosY += deltaY / zoom;
	}

	// fragment shader params
	glUniform2d(0, runtimeParams.viewPosX, runtimeParams.viewPosY);
	glUniform1d(1, zoom);
	if (isDoublePrecision == false)
	{
		glUniform1f(2, runtimeParams.a1);
		glUniform1f(3, runtimeParams.a2);
		glUniform1f(4, runtimeParams.a3);
	}

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Fractal2dEffect::drawGUI()
{
	ImGui::Begin("Startup params (Fractals)");
	ImGui::Text("Pan: Right mouse button");
	ImGui::Text("Zoom: Scroll");
	ImGui::Text("Fast Zoom: Left shift + scroll");
	ImGui::InputInt("Iterations", &startupParams.Iterations, 100, 1000);
	ImGui::InputInt("Anti Aliasing", &startupParams.AntiAliasing, 1, 1);
	ImGui::Checkbox("Double precision (can go deeper)", &startupParams.isDoublePrecision);
	ImGui::End();

	if (isDoublePrecision == false)
	{
		ImGui::Begin("Runtime params (Fractals)");
		ImGui::Text("f(z) = a3*z^3 + a2*z^2 + a1*z + c");
		ImGui::SliderFloat("a3", &runtimeParams.a3, -5.0, 5.0);
		ImGui::SliderFloat("a2", &runtimeParams.a2, -5.0, 5.0);
		ImGui::SliderFloat("a1", &runtimeParams.a1, -5.0, 5.0);
		ImGui::End();
	}
}

void Fractal2dEffect::restart()
{
	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &ssbo);
	glDeleteVertexArrays(1, &vao);

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
		isRightMouseBtnDown = true;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		isRightMouseBtnDown = false;
	}
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
}
