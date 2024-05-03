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
#include "light.h"

static constexpr int SCR_WIDTH = 1920;  // Screen width
static constexpr int SCR_HEIGHT = 1080; // Screen height
static constexpr float z_near = 0.1f;   // camera near
static constexpr float z_far = 1000.0f; // camera far

static unsigned int instancingBuffer = 0; // instancing buffer id
static unsigned int amount = 1000; // total amount of rocks
static float radius = 50.0f; // belt radius around mars
static float offset = 3.0f;	 // control the random displacement of each rock, choose a rational range to minimize rock collisions.
static float asteroidScale = 2.0f;	 // control the random displacement of each rock, choose a rational range to minimize rock collisions.

// PBR material textures id, set to 0 for safety
static unsigned int albedo = 0, normal = 0, metallic = 0, roughness = 0, ao = 0;
static float metallicScale = 1.0f; // Scale factor for metallic
static float roughnessScale = 1.0f; // Scale factor for roughness
static glm::vec3 albedoScale(1.0f, 1.0f, 1.0f); // Scale factor for albedo

void initModelMatricesAndRotationSpeeds(glm::mat4* modelMatrices, glm::vec3* rotationAxis, float* rotationSpeeds);
void setupInstancingBuffer(unsigned int& instancingBuffer, glm::mat4* modelMatrices, const Model& rock);
void setupPBRproperties(Shader& pbrShader, unsigned int& albedo, unsigned int& normal, unsigned int& metallic, unsigned int& roughness, unsigned int& ao);
void renderPBRmars(Shader& pbrShader, yzh::Sphere& sphere, glm::mat4& modelMatrix);

int main()
{
#ifdef DEBUG
	Timer timer;
	timer.start();
#endif 

	// shared pointer holding camera object. plane_near to 0.1f and plane_far to 100.0f by deault.
	// Camera initial position:(0.0f, 0.0f, 100.0f), initial direction: -z
	std::shared_ptr<Camera> camera = std::make_shared<Camera>(0.0f, 5.0f, 45.0f);

	// Global scene manager holding window and camera object instance with utility functions.
	// Notice: glfw and glw3 init in SceneManager constructor.
	// -------------------------------------------------------
	SceneManager scene_manager(SCR_WIDTH, SCR_HEIGHT, "CG assessment3", camera);

	// OpenGL global configs
    // ---------------------
	scene_manager.Enable(GL_DEPTH_TEST);

	// Load model(s)
	Model rock("res/models/rock/rock.obj");
	Model mars("res/models/planet/planet.obj");

	// Set VAO for geometry shape for later use
    //yzh::Quad quad;
    //yzh::Cube cube;
	yzh::Sphere sphere(64, 64);

	// Build & compile shader(s)
	Shader marsShader("res/shaders/instancing_mars.vs", "res/shaders/instancing_mars.fs");
	Shader rockShader("res/shaders/instancing_rock.vs", "res/shaders/instancing_rock.fs");
	Shader pbrShader("res/shaders/pbr_ibl.vert", "res/shaders/pbr_lighting_textured.frag");

	// prepare model matrices for amount of rocks
	glm::mat4* modelMatrices = new glm::mat4[amount];
	glm::vec3* rotationAxis = new glm::vec3[amount];
	float* rotationSpeeds = new float[amount];

	// Initialize matrices and speeds
	initModelMatricesAndRotationSpeeds(modelMatrices, rotationAxis, rotationSpeeds);

	// set up instancing buffer
	setupInstancingBuffer(instancingBuffer, modelMatrices, rock);

	// load textures for pbr rendering
	setupPBRproperties(pbrShader, albedo, normal, metallic, roughness, ao);

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

		// 1. Draw planet(mars) with pbr
		// -----------------------------
		model = glm::scale(model, glm::vec3(1.5f));
		pbrShader.Bind();
		pbrShader.SetMat4("projection", projection); // projection matrix
		pbrShader.SetMat4("view", view); // view matrix
		pbrShader.SetMat4("model", model);
		pbrShader.SetVec3("viewPos", camera->position); // view(eye) position
		renderPBRmars(pbrShader, sphere, model);

		// Update the rotation of each rock around its own random axis at a random speed.
		// Using deltaTime to ensure frame-rate independent rotation
		// ---------------------------------------------------------
		for (unsigned int i = 0; i < amount; i++) {
			glm::mat4 model = modelMatrices[i];

			// Rotate around the rock's own axis
			float angle = rotationSpeeds[i] * scene_manager.GetDeltaTime();
			model = glm::rotate(model, angle, rotationAxis[i]); // random angle & random axis
			modelMatrices[i] = model;
		}
		// Update the buffer with the new transformations
		glBindBuffer(GL_ARRAY_BUFFER, instancingBuffer);
		glBufferSubData(GL_ARRAY_BUFFER, 0, amount * sizeof(glm::mat4), &modelMatrices[0][0]);

		// 2. Draw amount of rocks
		// -----------------------
		rockShader.Bind();
		rockShader.SetMat4("projection", projection);
		rockShader.SetMat4("view", view);
		//rockShader.SetInt("texture_normal1", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, rock.GetMesh()[0].textures[0].id);

		// Special case: The rock model has only one mesh.
		// Directly bind its VAO and draw it instanced.
		// Note: This won't work for models with multiple meshes.
		glBindVertexArray(rock.GetMesh()[0].GetVAO());
		glDrawElementsInstanced(GL_TRIANGLES, (unsigned int)(rock.GetMesh()[0].indices.size()), GL_UNSIGNED_INT, 0, amount);
		glBindVertexArray(0);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(scene_manager.GetWindow());
		glfwPollEvents();
	}

	delete[] modelMatrices;
	delete[] rotationAxis;
	delete[] rotationSpeeds;
	glfwTerminate();
}

void initModelMatricesAndRotationSpeeds(glm::mat4* modelMatrices, glm::vec3* rotationAxis, float* rotationSpeeds) 
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
	std::uniform_real_distribution<float> scaleDis(0.05f, 0.2f);
	std::uniform_real_distribution<float> angleDis(0.0f, 360.0f);
	std::uniform_real_distribution<float> axisDis(0.0f, 1.0f);
	std::uniform_real_distribution<float> angleDistribution(4.0f, 8.0f);

	for (size_t i = 0; i < amount; i++) {
		// Calculate transformation
		float angle = static_cast<float>(i) / static_cast<float>(amount) * 360.0f;
		float x = sin(angle) * radius + dis(gen) * offset;
		float y = 0.6f * dis(gen) * offset;
		float z = cos(angle) * radius + dis(gen) * offset;
		glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
		float scale = scaleDis(gen) * asteroidScale;
		model = glm::scale(model, glm::vec3(scale));
		float rotAngle = angleDis(gen);
		glm::vec3 randomAxis(axisDis(gen), axisDis(gen), axisDis(gen));
		model = glm::rotate(model, glm::radians(rotAngle), randomAxis);

		// Store the transformations
		rotationAxis[i] = randomAxis;
		modelMatrices[i] = model;
		rotationSpeeds[i] = angleDistribution(gen);  // Set rotation speed
	}
}

void setupInstancingBuffer(unsigned int& instancingBuffer, glm::mat4* modelMatrices, const Model& rock) 
{
	// Generate and bind the instancing buffer
	glGenBuffers(1, &instancingBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, instancingBuffer);
	glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), modelMatrices, GL_STATIC_DRAW);

	// Set up vertex attributes for each mesh in the rock model
	for (unsigned int i = 0; i < rock.GetMesh().size(); i++) {
		unsigned int VAO = rock.GetMesh()[i].GetVAO();
		glBindVertexArray(VAO);

		// Define attributes for the model matrix
		for (int j = 0; j < 4; j++) {
			glEnableVertexAttribArray(3 + j);
			glVertexAttribPointer(3 + j, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * j));
			glVertexAttribDivisor(3 + j, 1);
		}

		glBindVertexArray(0);  // Unbind the VAO
	}
}

void setupPBRproperties(Shader& pbrShader, unsigned int& albedo, unsigned int& normal, unsigned int& metallic, unsigned int& roughness, unsigned int& ao)
{
	albedo = LoadTexture("res/textures/pbr/rusted_iron/albedo.png");
	normal = LoadTexture("res/textures/pbr/rusted_iron/normal.png");
	metallic = LoadTexture("res/textures/pbr/rusted_iron/metallic.png");
	roughness = LoadTexture("res/textures/pbr/rusted_iron/roughness.png");
	ao = LoadTexture("res/textures/pbr/rusted_iron/ao.png");

	pbrShader.Bind();
	pbrShader.SetInt("albedoMap", 0);
	pbrShader.SetInt("normalMap", 1);
	pbrShader.SetInt("metallicMap", 2);
	pbrShader.SetInt("roughnessMap", 3);
	pbrShader.SetInt("aoMap", 4);
}

void renderPBRmars(Shader& pbrShader, yzh::Sphere& sphere, glm::mat4& modelMatrix)
{
	pbrShader.SetVec3("lightColor", lightColor); // lighting info
	pbrShader.SetVec3("lightPosition", lightPosition); // lighting info

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
	pbrShader.SetFloat("roughnessScale", roughnessScale);
	pbrShader.SetFloat("metallicScale", metallicScale);
	pbrShader.SetVec3("albedoScale", albedoScale);
	pbrShader.SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(modelMatrix))));
	sphere.Render();
}