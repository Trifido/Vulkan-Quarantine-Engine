#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <stdexcept>
#include <vector>
#include <iostream>

#include "VulkanRenderer.h"

GLFWwindow* window;
VulkanRenderer vkRenderer;

void initWindow(std::string wName = "Test Window", const int width = 800, const int height = 600)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}

int main()
{
	//Create window
	initWindow("Test Window", 800, 600);

	//Create instance vkRenderer
	if (vkRenderer.init(window) == EXIT_FAILURE) return EXIT_FAILURE;

	float angle = 0.0f;
	float deltaTime = 0.0f;
	float lastTime = 0.0f;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		float now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		angle += 10.0f * deltaTime;
		if (angle > 360.0f) angle -= 360.0f;

		//vkRenderer.updateModel(glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f)));
		vkRenderer.draw();
	}

	vkRenderer.cleanup();

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}