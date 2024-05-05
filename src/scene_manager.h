#pragma once

#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif 

#include <string>
#include <iostream>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include "camera.h"

// A utility class holding window pointer & camera object
// 
// Example usage:
// SceneManager scene_manager();  // by using default parameters
// SceneManager scene_manager(1920, 1080, "hnzz", camera);
// 
// Utility function usages: load texture, enable/disable gl content, etc.
class SceneManager
{
public:
	using reference = SceneManager&;
	using poingter = SceneManager*;
	using const_reference = const SceneManager&;
	using const_pointer = const SceneManager*;

public:
	SceneManager(int width = 1920, int height = 1080, const std::string& title = "hnzz",
		glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f))
		: camera(std::make_shared<Camera>(position)),
		SCR_WIDTH(width), SCR_HEIGHT(height),
		lastX((float)width / 2.0f), lastY((float)height / 2.0f),
		deltaTime(0.0f), lastFrame(0.0f) {
		InitWindow(SCR_WIDTH, SCR_HEIGHT, title);
	}

	SceneManager(int width, int height, const std::string& title, std::shared_ptr<Camera> camera_ptr)
		:camera(camera_ptr), SCR_WIDTH(width), SCR_HEIGHT(height), 
		lastX((float)width / 2.0f), lastY((float)height / 2.0f),
		deltaTime(0.0f), lastFrame(0.0f) {
		InitWindow(SCR_WIDTH, SCR_HEIGHT, title);
	}

	SceneManager(const reference other) = delete;
	reference operator=(const reference other) = delete;
	SceneManager(SceneManager&& other) = delete;
	reference operator=(SceneManager&& other) = delete;

	~SceneManager() {
		glfwTerminate();
	}

	// Utility functions
	void Enable(GLenum content) { glEnable(content); }
	void Disable(GLenum content) { glDisable(content); }
	unsigned int LoadTexture(const std::string& path, bool isHDR = false);
	void UpdateDeltaTime(); // to calculate deltaTima each frame
	void CheckFramebufferStatus(unsigned int fbo, const std::string& framebufferName);

	// Callback functions
	static void framebuffer_size_callback_dispatch(GLFWwindow* window, int width, int height);
	void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	static void mouse_callback_dispatch(GLFWwindow* window, double xposIn, double yposIn);
	void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
	static void scroll_callback_dispatch(GLFWwindow* window, double xoffset, double yoffset);
	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	void ProcessInput();

	// Getter functions
	const auto& GetWindow() const{ return this->window; }
	const auto& GetCamera() const { return this->camera; }
	const auto& GetDeltaTime() const { return this->deltaTime; }
private:
	void InitWindow(int width, int height, const std::string& title);

private:
	GLFWwindow* window = nullptr;
	std::shared_ptr<Camera> camera; // camera pointer to keep the same camera object with external one
	bool enableCameraMovement = true;
	bool mouseButtonPressed = true;

	float lastX, lastY;
	float deltaTime, lastFrame;
	int SCR_WIDTH, SCR_HEIGHT;
};


void SceneManager::InitWindow(int width, int height, const std::string& title)
{
	try {
		if (!glfwInit())
			throw std::runtime_error("failed to init glfw");
		this->window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
		if (!window)
			throw std::runtime_error("failed to create window");

		glfwMakeContextCurrent(window);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebuffer_size_callback_dispatch);
		glfwSetCursorPosCallback(window, mouse_callback_dispatch);
		glfwSetScrollCallback(window, scroll_callback_dispatch);

		if (gl3wInit()) 
			throw std::runtime_error("failed to init GL3W");

	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return;
	}

	std::cout << "Use the following keys to navigate the scene:\n";
	std::cout << "W: Move forward\n";
	std::cout << "S: Move backward\n";
	std::cout << "A: Move left\n";
	std::cout << "D: Move right\n";
	std::cout << "Scroll to zoom in or out\n";
	std::cout << "Hold left mouse button & move mouse to look around\n";
	std::cout << "Press ESC to exit the program\n\n";
}

void SceneManager::UpdateDeltaTime() {
	float currentFrame = (float)glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
}

void SceneManager::framebuffer_size_callback_dispatch(GLFWwindow* window, int width, int height)
{
	SceneManager* sceneManager = static_cast<SceneManager*>(glfwGetWindowUserPointer(window));
	sceneManager->framebuffer_size_callback(window, width, height);
}

void SceneManager::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that SCR_WIDTH and 
	// SCR_HEIGHT will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	this->SCR_WIDTH = width;
	this->SCR_HEIGHT = height;
}

void SceneManager::mouse_callback_dispatch(GLFWwindow* window, double xposIn, double yposIn)
{
	SceneManager* sceneManager = static_cast<SceneManager*>(glfwGetWindowUserPointer(window));
	sceneManager->mouse_callback(window, xposIn, yposIn);
}

// glfw: whenever the mouse moves, this callback is called
void SceneManager::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	if (!enableCameraMovement)
		return;

	// Check if the left mouse button is pressed
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		float xpos = static_cast<float>(xposIn);
		float ypos = static_cast<float>(yposIn);

		if (mouseButtonPressed) {
			lastX = xpos;
			lastY = ypos;
			mouseButtonPressed = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

		lastX = xpos;
		lastY = ypos;

		camera->ProcessMouseMovement(xoffset, yoffset);
	}
	else
		mouseButtonPressed = true;
}

void SceneManager::scroll_callback_dispatch(GLFWwindow* window, double xoffset, double yoffset)
{
	SceneManager* sceneManager = static_cast<SceneManager*>(glfwGetWindowUserPointer(window));
	sceneManager->scroll_callback(window, xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void SceneManager::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera->ProcessMouseScroll(static_cast<float>(yoffset));
}

// Process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void SceneManager::ProcessInput()
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera->ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera->ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera->ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera->ProcessKeyboard(RIGHT, deltaTime);
}

// Utility function for loading a 2D texture from file
// Loading HDR texture when isHDR is true
unsigned int SceneManager::LoadTexture(const std::string& path, bool isHDR)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	void* data;

	// Using stbi_load or stbi_loadf for different texture type
	if (!isHDR) {
		data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
	}
	else {
		stbi_set_flip_vertically_on_load(true);
		data = stbi_loadf(path.c_str(), &width, &height, &nrComponents, 0);
	}

	if (data) {
		GLenum format;
		GLenum dataType = (isHDR) ? GL_FLOAT : GL_UNSIGNED_BYTE; // Set GL_FLOAT or GL_UNSIGNED_BYTE for different texture data type
		GLint internalFormat; // Using 16-bit float format for HDR, otherwise internalFormat equals to format

		if (nrComponents == 1) format = internalFormat = GL_RED;
		else if (nrComponents == 3) format = internalFormat = GL_RGB;
		else if (nrComponents == 4) format = internalFormat = GL_RGBA;
		else std::cerr << "Invalid texture format: Unsupported number of components!\n";

		if (isHDR) internalFormat = GL_RGB16F;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, data);
		if (!isHDR)
			glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (!isHDR)
			stbi_image_free(data);
		else
			stbi_image_free((float*)data);
	}
	else {
		std::cerr << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
		return -1;
	}

	return textureID;
}

void SceneManager::CheckFramebufferStatus(unsigned int fbo, const std::string& framebufferName)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "Framebuffer (" << framebufferName << ") with ID (" << fbo << ") is not complete! Error code: " << status << std::endl;
		// You can further translate 'status' into specific error strings if you want
	}
	else {
		std::cout << "Framebuffer (" << framebufferName << ") with ID (" << fbo << ") is complete." << std::endl;
	}
}

#endif // !SCENE_MANAGER_H