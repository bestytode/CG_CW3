// Assessment 3: textured PBR
//
// Explaination:
// PBR implemented by glsl. Albedo, roughness, metallic provided by textures.
// 
// To show how metallic and roughness affect pbr model surface,
// we have number of rows and number of colums with different control factors to render them all.
//
// Author: Zhenhuan
// Date: 2024/4/17

#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "camera.h"
#include "geometry_renderers.h"
#include "scene_manager.h"
#include "shader.h"
#include "timer.h"
#include "model.h"

// window configs
static constexpr int SCR_WIDTH = 1920, SCR_HEIGHT = 1080;

// row, col, spacing configs
static int nrRows = 7, nrColumns = 7;
static float spacing = 2.5;

// PBR material textures id, set to 0 for safety
static unsigned int albedo = 0, normal = 0, metallic = 0, roughness = 0, ao = 0;

// Scaling factors 
static float metallicScale = 1.0f; // Scale factor for metallic
static float roughnessScale = 1.0f; // Scale factor for roughness
static glm::vec3 albedoScale(1.0f, 1.0f, 1.0f); // Scale factor for albedo

int main()
{
#ifdef DEBUG
	Timer timer;
	timer.start();
#endif 

	// shared pointer holding camera object. plane_near to 0.1f and plane_far to 100.0f by deault.
	// Camera initial position:(0.0f, 0.0f, 6.0f), initial direction: -z
	std::shared_ptr<Camera> camera = std::make_shared<Camera>(0.0f, 0.0f, 6.0f);

	// Global scene manager holding window and camera object instance with utility functions.
	// Notice: glfw and glw3 init in SceneManager constructor.
	// -------------------------------------------------------
	SceneManager scene_manager(SCR_WIDTH, SCR_HEIGHT, "PBR", camera);
	scene_manager.Enable(GL_DEPTH_TEST);

	// Set VAO for geometry shape for later use
	//yzh::Quad quad;
	//yzh::Cube cube;
	yzh::Sphere sphere(64, 64);

	// build and compile shader(s)
	Shader shader("res/shaders/pbr_ibl.vert", "res/shaders/pbr_lighting_textured.frag");
	Shader shaderLight("res/shaders/debug_light.vs", "res/shaders/debug_light.fs");

	// lighting infos
	// --------------
	glm::vec3 lightPosition = glm::vec3(0.0f, 0.0f, 10.0f);
	glm::vec3 lightColor = glm::vec3(150.0f, 150.0f, 150.0f);

	// load PBR material textures
	// --------------------------
	albedo = LoadTexture("res/textures/pbr/rusted_iron/albedo.png");
	normal = LoadTexture("res/textures/pbr/rusted_iron/normal.png");
	metallic = LoadTexture("res/textures/pbr/rusted_iron/metallic.png");
	roughness = LoadTexture("res/textures/pbr/rusted_iron/roughness.png");
	ao = LoadTexture("res/textures/pbr/rusted_iron/ao.png");

	shader.Bind();
	shader.SetInt("albedoMap", 0);
	shader.SetInt("normalMap", 1);
	shader.SetInt("metallicMap", 2);
	shader.SetInt("roughnessMap", 3);
	shader.SetInt("aoMap", 4);

#ifdef DEBUG
	timer.stop();
#endif 

	while (!glfwWindowShouldClose(scene_manager.GetWindow())) {
		scene_manager.UpdateDeltaTime();
		scene_manager.ProcessInput();

		// Clear color and depth buffer
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// PBR rendering
		// -------------
		shader.Bind();
		glm::mat4 projection = glm::perspective(glm::radians(camera->fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera->GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);
		shader.SetMat4("projection", projection); // projection matrix
		shader.SetMat4("view", view); // view matrix
		shader.SetVec3("viewPos", camera->position); // view(eye) position
		shader.SetVec3("lightColor", lightColor); // lighting info
		shader.SetVec3("lightPosition", lightPosition); // lighting info

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, albedo);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, metallic);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, roughness);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, ao);

		// Scaling factors
		shader.SetFloat("roughnessScale", roughnessScale);
		shader.SetFloat("metallicScale", metallicScale);
		shader.SetVec3("albedoScale", albedoScale);

		// render rows * column number of spheres with varying metallic/roughness values
		// -----------------------------------------------------------------------------
		for (int row = 0; row < nrRows; row++) {
			for (int col = 0; col < nrColumns; col++) {
				float metallicValue = (float)row / (nrRows - 1);  // Metallic increases from top to bottom
				float roughnessValue = (float)col / (nrColumns - 1);  // Roughness increases from left to right
				roughnessValue = glm::clamp(roughnessValue, 0.05f, 1.0f); // Clamp roughness between 0.05 and 1.0

				// Set shader uniforms for PBR properties
				shader.SetFloat("metallicScale", metallicValue);
				shader.SetFloat("roughnessScale", roughnessValue);

				// Clamp the roughness to 0.05 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit on direct lighting.
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3((col - (nrColumns / 2)) * spacing, (row - (nrRows / 2)) * spacing, 0.0f));
				model = glm::scale(model, glm::vec3(0.5f));

				shader.SetMat4("model", model);
				shader.SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
				sphere.Render();
			}
		}

		// render light source 
		// -------------------
		shaderLight.Bind();
		shaderLight.SetMat4("projection", projection);
		shaderLight.SetMat4("view", view);

		model = glm::mat4(1.0f);
		model = glm::translate(model, lightPosition);
		model = glm::scale(model, glm::vec3(0.5f));
		shaderLight.SetMat4("model", model);
		sphere.Render();

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(scene_manager.GetWindow());
		glfwPollEvents();
	}

	glfwTerminate();
}