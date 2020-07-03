#define GLEW_STATIC

#include <vector>
#include <iostream>
#include <sstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "AbstractEffect.h"
#include "Particles2dGravityEffect.h"
#include "Particles2dNBodyGravityEffect.h"
#include "Fractal2dEffect.h"


bool isVsyncEnabled = true;

const int effectsCount = 3;
AbstractEffect* effect;
const char* effectNames[effectsCount] = 
{
	"2D One Attractor Gravity",
	"2D N-Body Gravity",
	"2D Fractals"
};
int currentEffectIndex = 2;

GLFWwindow* window;
bool isFullscreen = false;
int WindowWidth = 1280, WindowHeight = 800;
int WindowPosX = 100, WindowPosY = 100;

bool isGUI = true;


void useSelectedEffect()
{
	if (effect)
	{
		delete effect;
	}

	switch (currentEffectIndex)
	{
	case 0:
		effect = new Particles2dGravityEffect(window);
		break;
	case 1:
		effect = new Particles2dNBodyGravityEffect(window);
		break;
	case 2:
		effect = new Fractal2dEffect(window);
		break;
	default:
		throw "Not supported effect";
	}

	effect->initialize();
}

void toggleFullscreenWindowed()
{
	isFullscreen = !isFullscreen;

	if (isFullscreen)
	{
		glfwGetWindowPos(window, &WindowPosX, &WindowPosY);
		glfwGetWindowSize(window, &WindowWidth, &WindowHeight);

		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);

		if (effect)
		{
			effect->windowSizeCallback(mode->width, mode->height);
		}
	}
	else
	{
		glfwSetWindowMonitor(window, NULL, WindowPosX, WindowPosY, WindowWidth, WindowHeight, 0);

		if (effect)
		{
			effect->windowSizeCallback(WindowWidth, WindowHeight);
		}
	}
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
	{
		toggleFullscreenWindowed();
	}

	if (key == GLFW_KEY_G && action == GLFW_PRESS)
	{
		isGUI = !isGUI;
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		effect->hotReloadShaders();
	}

	effect->keyCallback(key, scancode, action, mode);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	effect->mouseButtonCallback(button, action, mods);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	effect->scrollCallback(xoffset, yoffset);
}

void windowSizeCallback(GLFWwindow* window, int width, int height)
{
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	glViewport(0, 0, w, h);

	if (effect)
	{
		effect->windowSizeCallback(width, height);
	}
}

void drawGUI()
{
	ImGui::Begin("Main controlls");
	ImGui::Text("Toggle GUI: G");
	ImGui::Text("Toggle fullscreen: F11");

	const char* currentLabel = effectNames[currentEffectIndex];
	if (ImGui::BeginCombo("Effects", currentLabel))
	{
		for (int n = 0; n < effectsCount; n++)
		{
			const bool isSelected = (currentEffectIndex == n);
			if (ImGui::Selectable(effectNames[n], isSelected))
			{
				currentEffectIndex = n;
				useSelectedEffect();
			}

			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::Button("Restart current effect"))
	{
		effect->restart();
	}
	ImGui::End();

	effect->drawGUI();
}

int main()
{
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	if (!isVsyncEnabled)
	{
		glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);
	}

	
	window = glfwCreateWindow(WindowWidth, WindowHeight, "Particles 2D", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		getchar();
		return -1;
	}
	glfwSetWindowPos(window, WindowPosX, WindowPosY);
	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetWindowSizeCallback(window, windowSizeCallback);
	
	windowSizeCallback(window, WindowWidth, WindowHeight);


	glewExperimental = GL_TRUE;
	if (glewInit()  != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		getchar();
		return -1;
	}

	try
	{
		useSelectedEffect();
	}
	catch (...)
	{
		std::cout << "Failed to initialize effect" << std::endl;
		delete effect;
		glfwTerminate();
		getchar();
		return -1;
	}


	// ------ GUI setup ------ //

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform / Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	int nbFrames = 0;
	const char* windowTitle = "Visual experiments";

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	// ------ main cycle ------ //

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT);

		io = ImGui::GetIO();

		effect->draw(io.DeltaTime);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		if (isGUI)
		{
			drawGUI();
		}
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (!isFullscreen)
		{
			nbFrames++;
			if (nbFrames >= 60)
			{
				std::stringstream ss;
				ss << windowTitle << " | " << io.Framerate << " FPS (" << io.DeltaTime * 1000 << " ms)";
				glfwSetWindowTitle(window, ss.str().c_str());

				nbFrames = 0;
			}
		}

		if (!isVsyncEnabled)
		{
			glFlush();
		}
		else
		{
			glfwSwapBuffers(window);
		}
	}

	// ------ clearing ------ //

	delete effect;
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// ------ terminate ------ //

	glfwDestroyWindow(window);
	glfwTerminate();
	
	return 0;
}