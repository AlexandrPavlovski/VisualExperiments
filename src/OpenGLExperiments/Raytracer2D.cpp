// attempt to recreate this beauty
// https://benedikt-bitterli.me/tantalum/tantalum.html

#include "Raytracer2D.h"
#include "Scene.h"

using namespace Raytracer2Dns;

Raytracer2D::Raytracer2D(GLFWwindow* window)
	:AbstractEffect(window)
{
	vertexShaderFileName = "Raytracer2D.vert";
	fragmentShaderFileName = "Raytracer2D.frag";

	startupParams = { };
	startupParams.simulationAreaWidth = 1280;
	startupParams.simulationAreaHeight = 800;

	runtimeParams = {};
	runtimeParams.maxPathLength = 1;
}

Raytracer2D::~Raytracer2D() { }

void Raytracer2D::initialize()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ssbo);
	glGenBuffers(1, &ssboDebug);

	// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	glGenFramebuffers(1, &floatFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, floatFramebuffer);
	// The texture we're going to render to
	glGenTextures(1, &floatTexture);
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, floatTexture);
	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, startupParams.simulationAreaWidth, startupParams.simulationAreaHeight, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	// Set "floatTexture" as our colour attachement #0
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, floatTexture, 0);
	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
	// Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw "framebuffer is not ok";
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLint newShaderProgram = createShaderProgramFromFiles();
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}

	shaderProgram = newShaderProgram;

	newShaderProgram = createShaderProgramFromFiles(
		"Raytracer2D_RenderToFloatTexture.vert",
		"Raytracer2D_RenderToFloatTexture.frag",
		std::vector<ShaderParam>(),
		std::vector<ShaderParam>());
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}
	floatTextureShaderProgram = newShaderProgram;

	newShaderProgram = createShaderProgramFromFiles(
		"Raytracer2D_DebugDraw.vert",
		"Raytracer2D_DebugDraw.frag",
		std::vector<ShaderParam>(),
		std::vector<ShaderParam>());
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}
	debugDrawShaderProgram = newShaderProgram;

	scene = new Scene(runtimeParams.maxPathLength);
	tracedLines = new Line[runtimeParams.maxPathLength];

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	isPaused = true;
	isDebug = true;
}

void Raytracer2D::draw(GLdouble deltaTime)
{
	handleInput();

	if (!isPaused || isAdvanceOneFrame)
	{
//for (int i = 0; i < maxLinesToDrawPerFrame; i += 4)
//{
//	currentFrameLinesToDraw[i + 0] = -600.0 + i / 1000;
//	currentFrameLinesToDraw[i + 1] = -200.0;
//	currentFrameLinesToDraw[i + 2] = -601.0 + i / 1000;
//	currentFrameLinesToDraw[i + 3] = 200.0;
//}
//raysTracedLastFrame = maxLinesToDrawPerFrame;
//GLint linesToDraw = maxLinesToDrawPerFrame;
		raysTracedLastFrame = 0;
		GLint linesToDraw = 0;
		Ray ray;
		while (linesToDraw <= maxLinesToDrawPerFrame - runtimeParams.maxPathLength)
		{
			scene->lightSource->getRay(ray);
			GLint tracedCount = scene->trace(ray, tracedLines, runtimeParams.maxPathLength);

			for (GLint i = 0; i < tracedCount; i++)
			{
				currentFrameLinesToDraw[(linesToDraw + i) * 4 + 0] = tracedLines[i].start.x;
				currentFrameLinesToDraw[(linesToDraw + i) * 4 + 1] = tracedLines[i].start.y;
				currentFrameLinesToDraw[(linesToDraw + i) * 4 + 2] = tracedLines[i].end.x;
				currentFrameLinesToDraw[(linesToDraw + i) * 4 + 3] = tracedLines[i].end.y;
			}

			raysTracedLastFrame++;
			linesToDraw += tracedCount;
		}

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, linesToDraw * 4 * sizeof(GLfloat), &currentFrameLinesToDraw[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, floatFramebuffer);
		glUseProgram(floatTextureShaderProgram);
		glUniform2i(0, startupParams.simulationAreaWidth, startupParams.simulationAreaHeight);
		glEnable(GL_BLEND);
		glDrawArrays(GL_LINES, 0, linesToDraw * 2);
		glDisable(GL_BLEND);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		raysTracedTotal += raysTracedLastFrame;
		frameCount++;
	}

	//glActiveTexture(GL_TEXTURE0);
	//GLfloat pixel[100 * 4];
	//glReadPixels(300, 300, 10, 10, GL_RGBA, GL_FLOAT, &pixel[0]);

	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shaderProgram);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, floatTexture);
	glUniform1i(0, 0);
	glUniform1f(1, 300.00f / raysTracedTotal); // exposure
	glUniform2i(2, startupParams.simulationAreaWidth, startupParams.simulationAreaHeight);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if (isDebug)
	{
		glUseProgram(debugDrawShaderProgram);
		glUniform2i(0, startupParams.simulationAreaWidth, startupParams.simulationAreaHeight);

		// obstacles
		glUniform4f(1, 1.0f, 0.0f, 0.0f, 1.0f);
		size_t drawPointsCount = scene->obstaclesDrawPoints.size();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboDebug);
		glBufferData(GL_SHADER_STORAGE_BUFFER, drawPointsCount * sizeof(Vec2), &scene->obstaclesDrawPoints[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glDrawArrays(GL_LINES, 0, drawPointsCount);

		// bvh
		glUniform4f(1, 1.0f, 1.0f, 1.0f, 1.0f);
		drawPointsCount = scene->bvhDrawPoints.size();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboDebug);
		glBufferData(GL_SHADER_STORAGE_BUFFER, drawPointsCount * sizeof(Vec2), &scene->bvhDrawPoints[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glDrawArrays(GL_LINES, 0, drawPointsCount);
	}

	isLeftMouseBtnPressed = false;
	isLeftMouseBtnReleased = false;
}

void Raytracer2D::drawGUI()
{
	ImGui::Begin("Startup params (Empty Effect)");
	ImGui::Text("Move attractor: A");
	ImGui::PushItemWidth(120);
	//ImGui::InputInt("Particles count", &startupParams.ParticlesCount, 100000, 1000000);
	ImGui::PopItemWidth();
	ImGui::End();

	ImGui::Begin("Runtime params (Empty Effect)");
	//ImGui::SliderFloat("Force scale", &runtimeParams.ForceScale, -1.0, 10.0);
	//ImGui::SliderFloat("Velocity damping", &runtimeParams.VelocityDamping, 0.9, 1.0);
	//ImGui::SliderFloat("Min distance", &runtimeParams.MinDistanceToAttractor, 0.0, 1000.0);
	//ImGui::SliderFloat("Time scale", &runtimeParams.TimeScale, 0.0, 10.0);
	//ImGui::ColorPicker4("", runtimeParams.Color, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoLabel);
	ImGui::End();
}

void Raytracer2D::restart()
{
	frameCount = 1;
	raysTracedTotal = 0;

	cleanup();
	initialize();
}

void Raytracer2D::cleanup()
{
	glDeleteTextures(1, &floatTexture);
	glDeleteProgram(floatTextureShaderProgram);
	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &ssbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteFramebuffers(1, &floatFramebuffer);

	delete[] tracedLines;
	delete scene;
}


void Raytracer2D::handleInput()
{
	//if (isLeftMouseBtnPressed)
	//{
	//	GLdouble cursorPosX = 0.0, cursorPosY = 0.0;
	//	glfwGetCursorPos(window, &cursorPosX, &cursorPosY);
	//	drawStartPos.x = cursorPosX;
	//	drawStartPos.y = cursorPosY;
	//}

	//if (isLeftMouseBtnReleased)
	//{
	//	GLdouble cursorPosX = 0.0, cursorPosY = 0.0;
	//	glfwGetCursorPos(window, &cursorPosX, &cursorPosY);
	//	scene->addObstacle(new LineObstacle(drawStartPos, Vec2(cursorPosX, cursorPosY)));
	//}
}


void Raytracer2D::keyCallback(int key, int scancode, int action, int mode)
{
	AbstractEffect::keyCallback(key, scancode, action, mode);

	if (key == GLFW_KEY_D && action == GLFW_PRESS)
	{
		isDebug = !isDebug;
	}
}

void Raytracer2D::mouseButtonCallback(int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		isLeftMouseBtnPressed = true;
		isLeftMouseBtnDown = true;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		isLeftMouseBtnReleased = true;
		isLeftMouseBtnDown = false;
	}
}

