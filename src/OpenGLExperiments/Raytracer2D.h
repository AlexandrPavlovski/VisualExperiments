#pragma once

#include "AbstractEffect.h"
#include "Scene.h"

#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

using namespace Raytracer2Dns;

class Raytracer2D : public AbstractEffect
{
public:
	Raytracer2D(GLFWwindow* window);

	virtual ~Raytracer2D();
	virtual void initialize();
	virtual void draw(GLdouble deltaTime);
	virtual void drawGUI();
	virtual void restart();
	virtual void cleanup();

	virtual void keyCallback(int key, int scancode, int action, int mode);
	virtual void mouseButtonCallback(int button, int action, int mods);

private:
	void handleInput();

	struct StartupParams
	{
		GLint simulationAreaWidth;
		GLint simulationAreaHeight;
	};

	struct RuntimeParams
	{
		GLbyte maxPathLength;
	};

	StartupParams startupParams;
	RuntimeParams runtimeParams;

	GLuint vao = 0, ssbo = 0, ssboDebug = 0;

	GLuint floatTextureShaderProgram = 0, debugDrawShaderProgram = 0;
	GLuint floatTexture = 0, texture1 = 0;

	GLuint floatFramebuffer = 0;

	// ==old ja hz==
	// only draw, no tracing -> 500k 75fps
	// only tracing, no draw -> 50k 30fps
	// without BVH: 50k rays 10 obstacles -> 72ms frame

	// ==release build, 50k rays, 10 obstacles==
	// no tracing -> 8.0ms (120fps cap)
	// BVH -> 12.9ms
	// perebor -> 8.2ms

	// ==release build, 50k rays, 50 obstacles==
	// no tracing -> 8.3ms
	// BVH -> 18.3ms
	// perebor -> 10.8ms

	// ==release build, 50k rays, 250 obstacles==
	// no tracing -> 8.4ms
	// BVH -> 144.0ms
	// perebor -> 40.3ms

	// ==release build, 50k rays, 2500 obstacles==
	// no tracing -> ms
	// BVH -> 1272.0ms
	// perebor -> 518ms
	static const GLuint maxLinesToDrawPerFrame = 500;
	GLfloat currentFrameLinesToDraw[maxLinesToDrawPerFrame * 4];
	GLint raysTracedLastFrame = 0;
	GLint raysTracedTotal = 0;

	bool isDebug = false;
	GLuint frameCount = 1;

	Scene *scene;
	Line *tracedLines;

	bool isLeftMouseBtnDown = false;
	bool isLeftMouseBtnPressed = false;
	bool isLeftMouseBtnReleased = false;
	Vec2 drawStartPos;
};
