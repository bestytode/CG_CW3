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

// Scene settings
constexpr int SCR_WIDTH = 1920;
constexpr int SCR_HEIGHT = 1080;

static void RenderScene(Shader& shader, yzh::Cube& cube);

int main()
{
#ifdef DEBUG
	Timer timer;
	timer.start();
#endif 

	// shared pointer holding camera object. plane_near to 0.1f and plane_far to 100.0f by deault.
	// Camera initial position:(0.0f, 0.0f, 3.0f), initial direction: -z
	std::shared_ptr<Camera> camera = std::make_shared<Camera>(0.0f, 0.0f, 3.0f);

	// Global scene manager holding window and camera object instance with utility functions.
	// Notice: glfw and glw3 init in SceneManager constructor.
	// -------------------------------------------------------
	SceneManager scene_manager(SCR_WIDTH, SCR_HEIGHT, "PBR", camera);
	scene_manager.Enable(GL_DEPTH_TEST);
	scene_manager.Enable(GL_CULL_FACE);

	// geometry renderer
	yzh::Sphere sphere(64, 64);
	yzh::Cube cube;

	// Create & config cube map
	unsigned int depthCubemap, depthCubeFBO;
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	glGenFramebuffers(1, &depthCubeFBO);
	glGenTextures(1, &depthCubemap);

	glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
	for (size_t i = 0; i < 6; i++)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindFramebuffer(GL_FRAMEBUFFER, depthCubeFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Shaders & textures configs
	Shader shader("res/shaders/point_shadow.vs", "res/shaders/point_shadow.fs");
	Shader simpleDepthShader("res/shaders/point_shadow_depth.vs", "res/shaders/point_shadow_depth.fs", "res/shaders/point_shadow_depth.gs");
	Shader lightShader("res/shaders/debug_light.vs", "res/shaders/debug_light.fs");
	unsigned int woodTexture = LoadTexture("res/textures/wood.png");

	shader.Bind();
	shader.SetInt("diffuseTexture", 0);
	shader.SetInt("depthMap", 1);

	// lighting info
	glm::vec3 lightPos(0.0f, 0.0f, 0.0f);

	// 0. create depth cubemap transformation matrices
	float near_plane = 1.0f;
	float far_plane = 25.0f;
	glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), float(SHADOW_WIDTH) / float(SHADOW_HEIGHT), near_plane, far_plane);

	std::vector<glm::vec3> directions = {
		glm::vec3(1.0, 0.0, 0.0), glm::vec3(-1.0, 0.0, 0.0),
		glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, -1.0, 0.0),
		glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 0.0, -1.0)
	};
	std::vector<glm::vec3> upVectors = {
		glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, -1.0, 0.0),
		glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 0.0, -1.0),
		glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, -1.0, 0.0)
	};
	std::vector<glm::mat4> shadowTransforms;
	for (int i = 0; i < 6; ++i)
		shadowTransforms.emplace_back(shadowProj * glm::lookAt(lightPos, lightPos + directions[i], upVectors[i]));

#ifdef DEBUG
	timer.stop();
#endif 

	while (!glfwWindowShouldClose(scene_manager.GetWindow())) {
		scene_manager.UpdateDeltaTime();
		scene_manager.ProcessInput();

		glClearColor(0.5f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 1. render scene to depth cubemap
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthCubeFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		simpleDepthShader.Bind();
		for (unsigned int i = 0; i < 6; ++i)
			simpleDepthShader.SetMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);

		simpleDepthShader.SetFloat("far_plane", far_plane);
		simpleDepthShader.SetVec3("lightPos", lightPos);
		RenderScene(simpleDepthShader, cube);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 2. render scene as normal 
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shader.Bind();
		glm::mat4 projection = glm::perspective(glm::radians(camera->fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera->GetViewMatrix();
		shader.SetMat4("projection", projection);
		shader.SetMat4("view", view);
		shader.SetVec3("lightPos", lightPos);
		shader.SetVec3("viewPos", camera->position);
		shader.SetInt("shadows", true); // hard-code enble shadow, can add toggle if using imgui
		shader.SetFloat("far_plane", far_plane);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, woodTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
		RenderScene(shader, cube);

		// Light visualization
		lightShader.Bind();
		glm::mat4 model_temp(1.0f);
		model_temp = glm::translate(model_temp, lightPos);
		model_temp = glm::scale(model_temp, glm::vec3(0.1f));
		lightShader.SetMat4("projection", projection);
		lightShader.SetMat4("view", view);
		lightShader.SetMat4("model", model_temp);
		sphere.Render();

		glfwSwapBuffers(scene_manager.GetWindow());
		glfwPollEvents();
	}

	glfwTerminate();
}

// Renders the 3D scene, accepting a shader as parameter
static void RenderScene(Shader& shader, yzh::Cube& cube)
{
	// room cube
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(5.0f));
	shader.SetMat4("model", model);
	glDisable(GL_CULL_FACE); // Disable culling here since we render 'inside' the cube
	shader.SetInt("reverse_normals", 1);
	cube.Render();
	shader.SetInt("reverse_normals", 0);
	glEnable(GL_CULL_FACE);

	// cubes
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(4.0f, -3.5f, 0.0));
	model = glm::scale(model, glm::vec3(0.5f));
	shader.SetMat4("model", model);
	cube.Render();

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 3.0f, 1.0));
	model = glm::scale(model, glm::vec3(0.75f));
	shader.SetMat4("model", model);
	cube.Render();

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-3.0f, -1.0f, 0.0));
	model = glm::scale(model, glm::vec3(0.5f));
	shader.SetMat4("model", model);
	cube.Render();

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-1.5f, 1.0f, 1.5));
	model = glm::scale(model, glm::vec3(0.5f));
	shader.SetMat4("model", model);
	cube.Render();

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-1.5f, 2.0f, -3.0));
	model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
	model = glm::scale(model, glm::vec3(0.75f));
	shader.SetMat4("model", model);
	cube.Render();
}