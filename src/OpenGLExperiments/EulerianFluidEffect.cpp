#include "EulerianFluidEffect.h"


EulerianFluidEffect::EulerianFluidEffect(GLFWwindow* window)
	:AbstractEffect(window)
{
	vertexShaderFileName = "EulerianFluidEffect.vert";
	fragmentShaderFileName = "EulerianFluidEffect.frag";

	startupParams = {};

	runtimeParams = {};
	runtimeParams.substeps = 1;

	isPaused = true;
}

EulerianFluidEffect::~EulerianFluidEffect()
{
	cleanup();
}


void EulerianFluidEffect::initialize()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	std::vector<float> velocityFieldsData(cellsCount + simulationAreaWidth + simulationAreaHeight + 1);
	//velocityFieldsData[25650] = 10000.0;

	CreateTextureField(&textureU0, GL_TEXTURE0, simulationAreaWidth + 1, simulationAreaHeight, &velocityFieldsData[0], 0);
	CreateTextureField(&textureV0, GL_TEXTURE1, simulationAreaWidth, simulationAreaHeight + 1, &velocityFieldsData[0], 1);
	CreateTextureField(&textureU1, GL_TEXTURE2, simulationAreaWidth + 1, simulationAreaHeight, &velocityFieldsData[0], 2);
	CreateTextureField(&textureV1, GL_TEXTURE3, simulationAreaWidth, simulationAreaHeight + 1, &velocityFieldsData[0], 3);

	int w = simulationAreaWidth;
	int h = simulationAreaHeight;
	std::vector<GLuint> cellTypeData(cellsCount, 1); // 1 -> fluid
	for (int i = 0;              i < w;          i++)    cellTypeData[i] = 0; // 0 -> solid
	for (int i = cellsCount - w; i < cellsCount; i++)    cellTypeData[i] = 0;
	for (int i = w;              i < cellsCount; i += w) cellTypeData[i] = 0;
	for (int i = w - 1;          i < cellsCount; i += w) cellTypeData[i] = 0;

	createSsbo(&ssboCellType, 4, cellsCount * sizeof(GLuint), &cellTypeData[0], GL_DYNAMIC_DRAW);
	createSsbo(&bufferTest, 9, 20000 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);


	GLint newShaderProgram = createShaderProgramFromFiles();
	if (newShaderProgram == -1)
	{
		throw "Initialize failed";
	}
	shaderProgram = newShaderProgram;

	std::vector<ShaderParam> emptyShaderParams
	{
	};
	createComputeShaderProgram(solveIncompressibilityOddCompShaderProgram, "SolveIncompressibility.comp", emptyShaderParams);
	createComputeShaderProgram(advectVelocitiesOddCompShaderProgram, "AdvectVelocities.comp", emptyShaderParams);
	std::vector<ShaderParam> evenShaderParams
	{
		// swapping inputs and outputs
		{ "#define U_fieldInputBunding 0",  "#define U_fieldInputBunding 2" },
		{ "#define V_fieldInputBunding 1",  "#define V_fieldInputBunding 3" },
		{ "#define U_fieldOutputBunding 2", "#define U_fieldOutputBunding 0" },
		{ "#define V_fieldOutputBunding 3", "#define V_fieldOutputBunding 1" },
	};
	createComputeShaderProgram(solveIncompressibilityEvenCompShaderProgram, "SolveIncompressibility.comp", evenShaderParams);
	createComputeShaderProgram(advectVelocitiesEvenCompShaderProgram, "AdvectVelocities.comp", evenShaderParams);

	oddShaderProgram = createShaderProgramFromFiles(emptyShaderParams, evenShaderParams);

	isOddFrame = true;
}

void EulerianFluidEffect::draw(GLdouble deltaTime)
{
	if (!isPaused || isAdvanceOneFrame)
	{
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		for (int subSteps = 0; subSteps < runtimeParams.substeps; subSteps++)
		{
			glUseProgram(isOddFrame ? solveIncompressibilityOddCompShaderProgram : solveIncompressibilityEvenCompShaderProgram);
			glUniform2i(0, simulationAreaWidth, simulationAreaHeight);

			int wgnx = ceil(workGroupsCountX / 2.0);
			int wgny = ceil(workGroupsCountY / 2.0);
			// separating work groups to avoid simultaneous writes on edges of groups working area
			glUniform2i(1, 0, 0);
			glDispatchCompute(wgnx, wgny, 1);
			glUniform2i(1, 1, 1);
			glDispatchCompute(wgnx, wgny, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			// TODO make it checker like pattern
			glUniform2i(1, 0, 1);
			glDispatchCompute(wgnx, wgny, 1);
			glUniform2i(1, 1, 0);
			glDispatchCompute(wgnx, wgny, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}

		glUseProgram(isOddFrame ? advectVelocitiesOddCompShaderProgram : advectVelocitiesEvenCompShaderProgram);
		glUniform2i(0, simulationAreaWidth, simulationAreaHeight);
		glDispatchCompute(workGroupsCountX, workGroupsCountY, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
//GLfloat* test = readFromBuffer<GLfloat>(1, bufferTest);
//std::vector<float> compute_data(cellsCount*2);
//glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, &compute_data[0]);

		isOddFrame = !isOddFrame;
	}

	glUseProgram(isOddFrame ? oddShaderProgram : shaderProgram);
	glUniform2i(0, simulationAreaWidth, simulationAreaHeight);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void EulerianFluidEffect::drawGUI()
{
	ImGui::Begin("Startup params (Eulerian Fluid)");
	ImGui::Text("Move attractor: A");
	ImGui::PushItemWidth(120);
	//ImGui::InputInt("Particles count", &startupParams.ParticlesCount, 100000, 1000000);
	ImGui::PopItemWidth();
	ImGui::End();

	ImGui::Begin("Runtime params (Eulerian Fluid)");
	ImGui::SliderInt("Substeps", &runtimeParams.substeps, 1.0, 128.0);
	//ImGui::SliderFloat("Force scale", &runtimeParams.ForceScale, -1.0, 10.0);
	//ImGui::SliderFloat("Velocity damping", &runtimeParams.VelocityDamping, 0.9, 1.0);
	//ImGui::SliderFloat("Min distance", &runtimeParams.MinDistanceToAttractor, 0.0, 1000.0);
	//ImGui::SliderFloat("Time scale", &runtimeParams.TimeScale, 0.0, 10.0);
	//ImGui::ColorPicker4("", runtimeParams.Color, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoLabel);
	ImGui::End();
}

void EulerianFluidEffect::restart()
{
	cleanup();
	initialize();
}

void EulerianFluidEffect::cleanup()
{
	glDeleteProgram(shaderProgram);
	glDeleteProgram(solveIncompressibilityOddCompShaderProgram);
	glDeleteTextures(1, &textureU0);
	glDeleteTextures(1, &textureV0);
	glDeleteTextures(1, &textureU1);
	glDeleteTextures(1, &textureV1);
	glDeleteBuffers(1, &ssboCellType);
	glDeleteVertexArrays(1, &vao);
}

void EulerianFluidEffect::CreateTextureField(GLuint *texture, GLenum textureEnum, GLsizei width, GLsizei height, const void *data, GLuint unit)
{
	glGenTextures(1, texture);
	glActiveTexture(textureEnum);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, data);
	glBindImageTexture(unit, *texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
}

template< typename T >
T* EulerianFluidEffect::readFromBuffer(int elemCount, GLuint ssbo)
{
	T* buf = new T[elemCount];
	T* p = (T*)glMapNamedBufferRange(ssbo, 0, elemCount * sizeof(T), GL_MAP_READ_BIT);
	memcpy(buf, p, elemCount * sizeof(T));
	bool b = glUnmapNamedBuffer(ssbo);
	if (!b)__debugbreak();
	return buf;
}
