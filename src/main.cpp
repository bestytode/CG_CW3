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
#include "config.h"
#include "PBR.h"
#include "instancing.h"

int main()
{
#ifdef DEBUG
	Timer timer;
	timer.start();
#endif 

	// Shared_ptr holding camera object. plane_near to 0.1f and plane_far to 1000.0f by deault.
	// Camera initial position: Config::camPos,direction: -z
	std::shared_ptr<Camera> camera = std::make_shared<Camera>(camPos);

	// Global scene manager holding window and camera object instance with utility functions.
	// Notice: glfw and glw3 init in SceneManager constructor.
	// -------------------------------------------------------
	SceneManager scene_manager(SCR_WIDTH, SCR_HEIGHT, "CG Assessment 3", camera);

	// OpenGL global configs
    // ---------------------
	scene_manager.Enable(GL_DEPTH_TEST);

	// Load model(s)
	// -------------
	Model rock("res/models/rock/rock.obj");
	Model mars("res/models/planet/planet.obj");

	// Set VAO for geometry shape for later use
    //yzh::Quad quad;
    //yzh::Cube cube;
	yzh::Sphere sphere(64, 64);

	// Build & compile shader(s)
	// -------------------------
	Shader marsShader("res/shaders/instancing_mars.vert", "res/shaders/instancing_mars.frag");
	Shader rockShader("res/shaders/instancing_rock.vert", "res/shaders/instancing_rock.frag");
	Shader pbrShader("res/shaders/pbr_ibl.vert", "res/shaders/pbr_lighting_textured.frag");

	// Initialize matrices and speeds
	InitModelMatricesAndRotationSpeeds(modelMatrices, rotationAxis, rotationSpeeds);

	// set up instancing buffer
	SetupInstancingBuffer(instancingBuffer, modelMatrices, rock);

	// load textures for pbr rendering
	LoadPBRMaterials(albedo, normal, metallic, roughness, ao);

#ifdef DEBUG
	timer.stop();
#endif 

	// Main render loop
	while (!glfwWindowShouldClose(scene_manager.GetWindow())) {
		scene_manager.UpdateDeltaTime();
		scene_manager.ProcessInput();

		// Render
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Configure transformation matrices
		glm::mat4 projection = glm::perspective(glm::radians(camera->fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, z_near, z_far);
		glm::mat4 view = camera->GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);

		// 1. Draw planet(mars) with PBR
		// -----------------------------
		model = glm::scale(model, glm::vec3(1.5f));
		pbrShader.Bind();
		pbrShader.SetMat4("projection", projection); 
		pbrShader.SetMat4("view", view); 
		pbrShader.SetMat4("model", model);
		pbrShader.SetVec3("viewPos", camera->position); // view(eye) position
		pbrShader.SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));

		pbrShader.SetInt("albedoMap", 0);
		pbrShader.SetInt("normalMap", 1);
		pbrShader.SetInt("metallicMap", 2);
		pbrShader.SetInt("roughnessMap", 3);
		pbrShader.SetInt("aoMap", 4);

		pbrShader.SetFloat("roughnessScale", roughnessScale);
		pbrShader.SetFloat("metallicScale", metallicScale);
		pbrShader.SetVec3("albedoScale", albedoScale);

		pbrShader.SetVec3("lightColor", lightColor); 
		pbrShader.SetVec3("lightPosition", lightPosition); 
		RenderPBRMars(pbrShader, sphere);
		
		// 2. Draw amount of rocks with instancing
		// ---------------------------------------
		model = glm::mat4(1.0f); // reset model matrix

		// Update the rotation of each rock around its own random axis at a random speed.
		// Using deltaTime to ensure frame-rate independent rotation
		UpdateModelMatrices(scene_manager.GetDeltaTime());

		rockShader.Bind();
		rockShader.SetMat4("projection", projection);
		rockShader.SetMat4("view", view);
		RenderInstancingRocks(rockShader, rock);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(scene_manager.GetWindow());
		glfwPollEvents();
	}

	delete[] modelMatrices;
	delete[] rotationAxis;
	delete[] rotationSpeeds;
	glfwTerminate();
}