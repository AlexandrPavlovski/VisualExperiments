#include "Fractal2dEffect.h"

Fractal2dEffect::Fractal2dEffect(GLFWwindow* window)
	:AbstractEffect(window)
{
	vertexShaderFilePath = "Fractal2dEffect.vert";
	fragmentShaderFilePath = "Fractal2dEffect.frag";

	initialParams = {};
	initialParams.viewZoom = 2.5;
	initialParams.viewPosX = -2.3;
	initialParams.viewPosY = -initialParams.viewZoom / 2;

	startupParams = {};
	startupParams.Iterations = 1000;

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
	// triangle strip
	trianglesData.push_back(-1.0);trianglesData.push_back(1.0);
	trianglesData.push_back(1.0);trianglesData.push_back(1.0);
	trianglesData.push_back(-1.0);trianglesData.push_back(-1.0);
	trianglesData.push_back(1.0);trianglesData.push_back(-1.0);

	viewZoom = initialParams.viewZoom;
	viewPosX = initialParams.viewPosX;
	viewPosY = initialParams.viewPosY;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, trianglesData.size() * sizeof(GLfloat), &trianglesData[0], GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	std::string iterStr = "#define iterations " + std::to_string(startupParams.Iterations);
	const char* iterChar = iterStr.c_str();
	std::vector<ShaderParams> fragShaderParams
	{
		ShaderParams {"#define iterations 0", iterChar}
	};
	GLuint newShaderProgram = createShaderProgramFromFiles(std::vector<ShaderParams>(), fragShaderParams);
	if (newShaderProgram == 0)
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

	GLfloat zoom = windowHeight / viewZoom;

	if (isLeftMouseBtnDown)
	{
		GLfloat deltaX = oldCursorPosX - cursorPosX;
		GLfloat deltaY = cursorPosY - oldCursorPosY;

		viewPosX += deltaX / zoom;
		viewPosY += deltaY / zoom;
	}

	// fragment shader params
	glUniform2f(0, viewPosX, viewPosY);
	glUniform1f(1, zoom);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Fractal2dEffect::drawGUI()
{
	ImGui::Begin("Startup params");
	ImGui::Text("Pan: Left mouse button");
	ImGui::Text("Zoom: Scroll");
	ImGui::Text("Fast Zoom: Left shift + scroll");
	ImGui::InputInt("Iteratoins", &startupParams.Iterations, 100, 1000);
	ImGui::End();

	//ImGui::Begin("Runtime params");
	//ImGui::SliderFloat("Force scale", &runtimeParams.ForceScale, -1.0, 10.0);
	//ImGui::SliderFloat("Velocity damping", &runtimeParams.VelocityDamping, 0.9, 1.0);
	//ImGui::SliderFloat("Min distance", &runtimeParams.MinDistanceToAttractor, 0.0, 1000.0);
	//ImGui::SliderFloat("Time scale", &runtimeParams.TimeScale, 0.0, 10.0);
	//ImGui::ColorPicker4("", runtimeParams.Color, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoLabel);
	//ImGui::End();
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
		isLeftMouseBtnDown = true;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		isLeftMouseBtnDown = false;
	}
}

void Fractal2dEffect::scrollCallback(double xoffset, double yoffset)
{
	// this is needed because in shader (0, 0) is bottom left with positive direction up
	// while in GLFW (0, 0) is top left with bositive direction down
	GLdouble transfomedCursorPosY = windowHeight - cursorPosY;

	GLfloat zoom = windowHeight / viewZoom;
	GLfloat beforeZoomCursorPosX = cursorPosX / zoom + viewPosX;
	GLfloat	beforeZoomCursorPosY = transfomedCursorPosY / zoom + viewPosY;

	GLfloat multiplier = 1 + zoomSpeed / 10;
	if (yoffset < 0)
	{
		viewZoom *= multiplier;
	}
	else
	{
		viewZoom /= multiplier;
	}

	zoom = windowHeight / viewZoom;
	GLfloat afterZoomCursorPosX = cursorPosX / zoom + viewPosX;
	GLfloat	afterZoomCursorPosY = transfomedCursorPosY / zoom + viewPosY;

	viewPosX += beforeZoomCursorPosX - afterZoomCursorPosX;
	viewPosY += beforeZoomCursorPosY - afterZoomCursorPosY;
}
