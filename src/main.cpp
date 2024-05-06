#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <cmath>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "camera.h"
#include "geometry_renderers.h"
#include "scene_manager.h"
#include "shader.h"
#include "timer.h"
#include "model.h"
#include "config.h"
#include "pbr.h"
#include "instancing.h"
#include "bloom.h"
#include "skybox.h"

int main()
{
	Timer timer;
	timer.start();

	// Shared_ptr holding camera object. plane_near to 0.1f and plane_far to 1000.0f by deault.
	// Camera initial position: Config::camPos,direction: -z
	std::shared_ptr<Camera> camera = std::make_shared<Camera>(glm::vec3(0.0f, 5.0f, 45.0f));

	// Global scene manager holding window and camera object instance with utility functions.
	// Notice: glfw and glw3 init in SceneManager constructor.
	// -------------------------------------------------------
	SceneManager scene_manager(SCR_WIDTH, SCR_HEIGHT, "CG Assessment 3", camera);

	// OpenGL global configs
    // ---------------------
	scene_manager.Enable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);

	// Load model(s)
	// -------------
	Model rock("res/models/rock/rock.obj");
	Model nanosuit("res/models/nanosuit/nanosuit.obj");
	//Model mars("res/models/planet/planet.obj");

	// Set VAO for geometry shape for later use
    yzh::Quad quad;
    //yzh::Cube cube;
	yzh::Sphere sphere;

	// Build & compile shader(s)
	// -------------------------
	Shader rockShader("res/shaders/instancing_rock.vert", "res/shaders/instancing_rock.frag");
	Shader planetPBRShader("res/shaders/pbr.vert", "res/shaders/pbr.frag");
	Shader skyboxShader("res/shaders/skybox.vert", "res/shaders/skybox.frag");
	Shader bloomShader("res/shaders/debug_light.vs", "res/shaders/debug_light.fs");
	Shader nanosuitShader("res/shaders/nanosuit.vert", "res/shaders/nanosuit.frag");

	// Initialize matrices and speeds
	InitModelMatricesAndRotationSpeeds(modelMatrices, rotationAxis, rotationSpeeds);

	// set up instancing buffer
	SetupInstancingBuffer(instancingBuffer, modelMatrices, rock);

	// load textures for pbr rendering
	LoadPBRMaterials(albedo, normal, metallic, roughness, ao);

    // Set up sky box vao, vbo. load cube map textures
	SetupSkybox(skyboxVAO, skyboxVBO);

	// setup nanosuit
	glm::mat4 nanosuitModel = glm::mat4(1.0f);
	nanosuitModel = glm::translate(nanosuitModel, glm::vec3(0.0f, 0.0f, 12.0f));
	nanosuitModel = glm::scale(nanosuitModel, glm::vec3(0.25f));
	timer.stop();

	// Main render loop
	while (!glfwWindowShouldClose(scene_manager.GetWindow())) {
		scene_manager.UpdateDeltaTime();
		scene_manager.ProcessInput();
		float time = (float)glfwGetTime();
		
		// Render
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		lightPosition = UpdatePositionalLight(time);
		directionalLightDirection = UpdateDirectionalLight(0.2f * time);

		// Configure transformation matrices
		glm::mat4 projection = glm::perspective(glm::radians(camera->fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, z_near, z_far);
		glm::mat4 view = camera->GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);

		// 1. Render sky box
		model = glm::mat4(1.0f); // reset model matrix
		glm::mat4 skyview = glm::mat4(glm::mat3(camera->GetViewMatrix())); // Important: remove translation from the view matrix

		skyboxShader.Bind();
		skyboxShader.SetInt("skybox", 0);
		skyboxShader.SetMat4("view", skyview);
		skyboxShader.SetMat4("projection", projection);
		skyboxShader.SetMat4("model", model);
		RenderSkybox(skyboxShader);

		//2. Draw planet(mars) with PBR
		//-----------------------------
		model = glm::mat4(1.0f);
		model = glm::scale(model, glm::vec3(10.0f)); // radius 10.0f
		planetPBRShader.Bind();
		planetPBRShader.SetMat4("projection", projection);
		planetPBRShader.SetMat4("view", view);
		planetPBRShader.SetMat4("model", model);
		planetPBRShader.SetVec3("viewPos", camera->position); // view(eye) position
		planetPBRShader.SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));

		planetPBRShader.SetInt("albedoMap", 0);
		planetPBRShader.SetInt("normalMap", 1);
		planetPBRShader.SetInt("metallicMap", 2);
		planetPBRShader.SetInt("roughnessMap", 3);
		planetPBRShader.SetInt("aoMap", 4);

		planetPBRShader.SetFloat("roughnessScale", roughnessScale);
		planetPBRShader.SetFloat("metallicScale", metallicScale);
		planetPBRShader.SetVec3("albedoScale", albedoScale);
		planetPBRShader.SetFloat("ka", Ka);

		planetPBRShader.SetVec3("lightColor", lightColor);
		planetPBRShader.SetVec3("lightPosition", lightPosition);
		planetPBRShader.SetVec3("directionalLightDirection", directionalLightDirection);
		planetPBRShader.SetVec3("directionalLightColor", directionalLightColor);
		planetPBRShader.SetFloat("directionalLightScale", directionalLightScale);
		RenderPBRMars(planetPBRShader, sphere);

		// 3. Draw amount of rocks with instancing
		// ---------------------------------------
		model = glm::mat4(1.0f); // reset model matrix

		// Update the rotation of each rock around its own random axis at a random speed.
		// Using deltaTime to ensure frame-rate independent rotation
		UpdateModelMatrices(scene_manager.GetDeltaTime());

		rockShader.Bind();
		rockShader.SetMat4("projection", projection);
		rockShader.SetMat4("view", view);
		rockShader.SetFloat("ka", Ka);
		RenderInstancingRocks(rockShader, rock);

		// 4. Render nanosuit.obj
		// ------------------------------------------
		nanosuitShader.Bind();
		if (toggleNanosuitMovement) {
			if (moveForward) nanosuitModel = glm::translate(nanosuitModel, glm::vec3(0.0f, 0.0f, -0.1f));
			if (moveBackward) nanosuitModel = glm::translate(nanosuitModel, glm::vec3(0.0f, 0.0f, 0.1f));
			if (moveLeft) nanosuitModel = glm::translate(nanosuitModel, glm::vec3(-0.1f, 0.0f, 0.0f));
			if (moveRight) nanosuitModel = glm::translate(nanosuitModel, glm::vec3(0.1f, 0.0f, 0.0f));
			if (moveUp) nanosuitModel = glm::translate(nanosuitModel, glm::vec3(0.0f, 0.1f, 0.0f));
			if (moveDown) nanosuitModel = glm::translate(nanosuitModel, glm::vec3(0.0f, -0.1f, 0.0f));
			nanosuitModel = glm::rotate(nanosuitModel, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		}
		nanosuitShader.SetMat4("projection", projection);
		nanosuitShader.SetMat4("view", view);
		nanosuitShader.SetMat4("model", nanosuitModel);

		nanosuitShader.SetVec3("viewPos", camera->position);
		nanosuitShader.SetVec3("lightColor", lightColor);
		nanosuitShader.SetVec3("lightPosition", lightPosition);
		nanosuitShader.SetVec3("directionalLightDirection", directionalLightDirection);
		nanosuitShader.SetVec3("directionalLightColor", directionalLightColor);
		nanosuitShader.SetFloat("directionalLightScale", directionalLightScale);

		nanosuitShader.SetFloat("ka", Ka);
		nanosuitShader.SetFloat("ks", Ks);
		nanosuitShader.SetFloat("shininess", Ns);
		nanosuitShader.SetFloat("kd", Kd);
		nanosuit.Render(nanosuitShader, {"texture_diffuse", "texture_specular"});

		// 5. Render light source with bloom
		// ---------------------------------
		model = glm::mat4(1.0f); // reset model matrix
		model = glm::translate(model, lightPosition);
		model = glm::scale(model, glm::vec3(0.5f));
		bloomShader.Bind();
		bloomShader.SetMat4("projection", projection);
		bloomShader.SetMat4("view", view);
		bloomShader.SetMat4("model", model);
		bloomShader.SetVec3("lightColor", lightColor);
		RenderBloomLightSource(bloomShader, sphere);
		
		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(scene_manager.GetWindow());
		glfwPollEvents();
	}

	delete[] modelMatrices;
	delete[] rotationAxis;
	delete[] rotationSpeeds;
	glfwTerminate();
}