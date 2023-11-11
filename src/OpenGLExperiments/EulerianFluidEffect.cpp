//#define PROFILE


#include <chrono>
#include "EulerianFluidEffect.h"


EulerianFluidEffect::EulerianFluidEffect(GLFWwindow* window)
	:AbstractEffect(window)
{
	vertexShaderFileName = "EulerianFluidEffect.vert";
	fragmentShaderFileName = "EulerianFluidEffect.frag";

	startupParams = {};

	runtimeParams = {};
	runtimeParams.substeps = 64;

	isPaused = false;
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
	//CreateTextureField(&textureU1, GL_TEXTURE2, simulationAreaWidth + 1, simulationAreaHeight, &velocityFieldsData[0], 2);
	//CreateTextureField(&textureV1, GL_TEXTURE3, simulationAreaWidth, simulationAreaHeight + 1, &velocityFieldsData[0], 3);
	//CreateTextureField(&textureU0, GL_TEXTURE0, simulationAreaWidth + 1, simulationAreaHeight, &velocityFieldsData[0], 0);
	//for (int i = 0; i < velocityFieldsData.size(); i++) velocityFieldsData[i] = 1;
	//CreateTextureField(&textureV0, GL_TEXTURE1, simulationAreaWidth, simulationAreaHeight + 1, &velocityFieldsData[0], 1);

	int w = simulationAreaWidth;
	int h = simulationAreaHeight;
	std::vector<GLuint> cellTypeData(cellsCount, 1); // 1 -> fluid
	for (int i = 0;              i < w;          i++)    cellTypeData[i] = 0; // 0 -> solid
	for (int i = cellsCount - w; i < cellsCount; i++)    cellTypeData[i] = 0;
	for (int i = w;              i < cellsCount; i += w) cellTypeData[i] = 0;
	for (int i = w - 1;          i < cellsCount; i += w) cellTypeData[i] = 0;

	createSsbo(&ssboV0, 1, cellsCount * sizeof(GLfloat), &velocityFieldsData[0], GL_DYNAMIC_DRAW);
	createSsbo(&ssboU1, 2, cellsCount * sizeof(GLfloat), &velocityFieldsData[0], GL_DYNAMIC_DRAW);
	createSsbo(&ssboV1, 3, cellsCount * sizeof(GLfloat), &velocityFieldsData[0], GL_DYNAMIC_DRAW);
	createSsbo(&ssboSmoke1, 5, cellsCount * sizeof(GLfloat), &velocityFieldsData[0], GL_DYNAMIC_DRAW);
	velocityFieldsData[300 * simulationAreaWidth + 300] = 10000.0;
	createSsbo(&ssboU0, 0, cellsCount * sizeof(GLfloat), &velocityFieldsData[0], GL_DYNAMIC_DRAW);
	createSsbo(&ssboSmoke0, 4, cellsCount * sizeof(GLfloat), &velocityFieldsData[0], GL_DYNAMIC_DRAW);

	createSsbo(&ssboCellType, 6, cellsCount * sizeof(GLuint), &cellTypeData[0], GL_DYNAMIC_DRAW);

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
	createComputeShaderProgram(advectSmokeOddCompShaderProgram, "AdvectSmoke.comp", emptyShaderParams);
	std::vector<ShaderParam> evenShaderParams
	{
		// swapping inputs and outputs
		{ "#define U_fieldInputBinding 0",  "#define U_fieldInputBinding 2" },
		{ "#define V_fieldInputBinding 1",  "#define V_fieldInputBinding 3" },
		{ "#define U_fieldOutputBinding 2", "#define U_fieldOutputBinding 0" },
		{ "#define V_fieldOutputBinding 3", "#define V_fieldOutputBinding 1" },

		{ "#define U_fieldBufferI UI", "#define U_fieldBufferI UO" },
		{ "#define V_fieldBufferI VI", "#define V_fieldBufferI VO" },
		{ "#define U_fieldBufferO UO", "#define U_fieldBufferO UI" },
		{ "#define V_fieldBufferO VO", "#define V_fieldBufferO VI" },
	};
	createComputeShaderProgram(solveIncompressibilityEvenCompShaderProgram, "SolveIncompressibility.comp", evenShaderParams);
	createComputeShaderProgram(advectVelocitiesEvenCompShaderProgram, "AdvectVelocities.comp", evenShaderParams);

	std::vector<ShaderParam> evenSmokeShaderParams
	{
		// swapping inputs and outputs
		{ "#define smoke_fieldInputBinding 4",  "#define smoke_fieldInputBinding 5" },
		{ "#define smoke_fieldOutputBinding 5",  "#define smoke_fieldOutputBinding 4" },

		{ "#define smoke_fieldBufferI SI", "#define smoke_fieldBufferI SO" },
		{ "#define smoke_fieldBufferO SO", "#define smoke_fieldBufferO SI" },

		{ "#define U_fieldInputBinding 2",  "#define U_fieldInputBinding 0" },
		{ "#define V_fieldInputBinding 3",  "#define V_fieldInputBinding 1" },

		{ "#define U_fieldBufferI UO", "#define U_fieldBufferI UI" },
		{ "#define V_fieldBufferI VO", "#define V_fieldBufferI VI" },
	};
	createComputeShaderProgram(advectSmokeEvenCompShaderProgram, "AdvectSmoke.comp", evenSmokeShaderParams);

	std::vector<ShaderParam> evenShaderParams2
	{
		// swapping inputs and outputs
		{ "#define U_fieldInputBinding 2",  "#define U_fieldInputBinding 0" },
		{ "#define V_fieldInputBinding 3",  "#define V_fieldInputBinding 1" },

		{ "#define U_fieldBufferI UO", "#define U_fieldBufferI UI" },
		{ "#define V_fieldBufferI VO", "#define V_fieldBufferI VI" },

		{ "#define smoke_fieldInputBinding 5",  "#define smoke_fieldInputBinding 4" },
		{ "#define smoke_fieldBufferI SO", "#define smoke_fieldBufferI SI" },
	};
	evenShaderProgram = createShaderProgramFromFiles(emptyShaderParams, evenShaderParams2);

	isOddFrame = true; // frame numbers are starting from 1
}

void EulerianFluidEffect::draw(GLdouble deltaTime)
{
#ifdef PROFILE
GLuint queries[queriesSize];
glGenQueries(queriesSize, queries);

auto tBegin = std::chrono::steady_clock::now();
#endif

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	if (!isPaused || isAdvanceOneFrame)
	{

#ifdef PROFILE
glBeginQuery(GL_TIME_ELAPSED, queries[0]);
#endif

		for (int substep = 0; substep < runtimeParams.substeps; substep++)
		{
			glUseProgram(isOddFrame ? solveIncompressibilityOddCompShaderProgram : solveIncompressibilityEvenCompShaderProgram);
			glUniform2i(0, simulationAreaWidth, simulationAreaHeight);
			glUniform1i(2, substep == runtimeParams.substeps - 1); // isLastSubstep

			// separating work groups to avoid simultaneous writes on edges of groups working area
			glUniform1i(1, 0);
			glDispatchCompute(workGroupsCountX, workGroupsCountY, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
//GLfloat* test = readFromBuffer<GLfloat>(20000, bufferTest);
//int i = 0;
			glUniform1i(1, 1);
			glDispatchCompute(workGroupsCountX, workGroupsCountY, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
glBeginQuery(GL_TIME_ELAPSED, queries[1]);
#endif

		glUseProgram(isOddFrame ? advectVelocitiesOddCompShaderProgram : advectVelocitiesEvenCompShaderProgram);
		glUniform2i(0, simulationAreaWidth, simulationAreaHeight);
		glDispatchCompute(workGroupsCountX * 2, workGroupsCountY, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		glUseProgram(isOddFrame ? advectSmokeOddCompShaderProgram : advectSmokeEvenCompShaderProgram);
		glUniform2i(0, simulationAreaWidth, simulationAreaHeight);
		glDispatchCompute(workGroupsCountX * 2, workGroupsCountY, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
//GLfloat* test = readFromBuffer<GLfloat>(20000, bufferTest);
//int i = 0;

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
#endif

		isOddFrame = !isOddFrame;
	}

#ifdef PROFILE
glBeginQuery(GL_TIME_ELAPSED, queries[2]);
#endif

	glUseProgram(isOddFrame ? evenShaderProgram : shaderProgram);
	glUniform2f(0, simulationAreaWidth, simulationAreaHeight);
	glUniform2f(1, windowWidth, windowHeight);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#ifdef PROFILE
glEndQuery(GL_TIME_ELAPSED);
auto tEnd = std::chrono::steady_clock::now();

currentlyAveragedFrames++;

GLint done = 0;
while (!done) { // waiting for GPU
	glGetQueryObjectiv(queries[queriesSize - 1], GL_QUERY_RESULT_AVAILABLE, &done);
}

GLuint64 elapsedTimes[queriesSize];
for (int i = 0; i < queriesSize; i++)
{
	glGetQueryObjectui64v(queries[i], GL_QUERY_RESULT, &elapsedTimes[i]);
}

float d = 1000000.0; // nanoseconds to miliseconds
accumulatedTimeCpu += std::chrono::duration_cast<std::chrono::nanoseconds>(tEnd - tBegin).count() / d;
averageFrameTimeCpu = accumulatedTimeCpu / (float)currentlyAveragedFrames;
for (int i = 0; i < queriesSize; i++)
{
	accumulatedTimeGpu[i] += elapsedTimes[i] / d;
	averageFrameTimeGpu[i] = accumulatedTimeGpu[i] / (float)currentlyAveragedFrames;
}

if (currentlyAveragedFrames % framesToAvegare == 0)
{
	currentlyAveragedFrames = 0;
	accumulatedTimeCpu = 0.0;
	for (int i = 0; i < queriesSize; i++)
	{
		accumulatedTimeGpu[i] = 0.0;
	}
}
#endif
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

#ifdef PROFILE
ImGui::Begin("Average frame time, ms");
ImGui::Text(("CPU - " + std::to_string(averageFrameTimeCpu)).c_str());

ImGui::Text(("Incompressibility - " + std::to_string(averageFrameTimeGpu[0])).c_str());
ImGui::Text(("Advections - " + std::to_string(averageFrameTimeGpu[1])).c_str());
ImGui::Text(("Draw - "              + std::to_string(averageFrameTimeGpu[2])).c_str());
ImGui::End();
#endif
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
	glDeleteProgram(solveIncompressibilityEvenCompShaderProgram);
	glDeleteProgram(advectVelocitiesOddCompShaderProgram);
	glDeleteProgram(advectVelocitiesEvenCompShaderProgram);
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
