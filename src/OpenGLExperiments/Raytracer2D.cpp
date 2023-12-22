#include "Raytracer2D.h"

# define PI 3.14159265358979323846

Raytracer2D::Raytracer2D(GLFWwindow* window)
	:AbstractEffect(window)
{
	vertexShaderFileName = "Raytracer2D.vert";
	fragmentShaderFileName = "Raytracer2D.frag";

	startupParams = { };
	startupParams.simulationAreaWidth = 1280;
	startupParams.simulationAreaHeight = 800;

	runtimeParams = {};
}

Raytracer2D::~Raytracer2D()
{
	cleanup();
}

void Raytracer2D::initialize()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ssbo);

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
		return;
	}

	//glGenTextures(1, &texture1);
	//glBindTexture(GL_TEXTURE_2D, texture1);
	//// set the texture wrapping parameters
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//GL_CLAMP_TO_BORDER
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//// set texture filtering parameters
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//GLfloat data[100 * 100 * 3];// = { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
	////std::fill_n(data, 100 * 100 * 3, 1.0f);
	////data[0] = 1.0f;
	////data[1] = 0.0f;
	////data[2] = 0.0f;
	//for (int i = 0; i < 100; i++)
	//for (int j = 0; j < 100; j++)
	//{
	//	int t = j + i * 100;
	//	data[t * 3 + 0] = j / 100.0f;
	//	data[t * 3 + 1] = j / 100.0f;
	//	data[t * 3 + 2] = j / 100.0f;
	//}
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 100, 100, 0, GL_RGB, GL_FLOAT, data);


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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

void Raytracer2D::draw(GLdouble deltaTime)
{
	if (isPaused && !isAdvanceOneFrame)
		return;

	frameCount++;

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	for (int i = 0; i < linesPerFrameCount; i++)
	{
		linesForFrame[i * 4] = 0.0;
		linesForFrame[i * 4 + 1] = 0.0;
		GLfloat theta = (rand() / (GLfloat)RAND_MAX) * 2 * PI;
		linesForFrame[i * 4 + 2] = 1000 * cos(theta);
		linesForFrame[i * 4 + 3] = 1000 * sin(theta);
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, linesPerFrameCount * 4 * sizeof(GLfloat), &linesForFrame[0], GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, floatFramebuffer);
	glUseProgram(floatTextureShaderProgram);
	glUniform2i(0, startupParams.simulationAreaWidth, startupParams.simulationAreaHeight);
	glDrawArrays(GL_LINES, 0, linesPerFrameCount * 4);

	//glActiveTexture(GL_TEXTURE0);
	//GLfloat pixel[100 * 4];
	//glReadPixels(300, 300, 10, 10, GL_RGBA, GL_FLOAT, &pixel[0]);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(shaderProgram);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, floatTexture);
	glUniform1i(0, 0);
	glUniform1ui(1, frameCount);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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
	cleanup();
	initialize();
}

void Raytracer2D::cleanup()
{
	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &ssbo);
	glDeleteVertexArrays(1, &vao);
}
