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

static constexpr int SCR_WIDTH = 1920;  // Screen width
static constexpr int SCR_HEIGHT = 1080; // Screen height

static unsigned int amount = 5000; // total amount of rocks
static float radius = 50.0f; // belt radius around mars
static float offset = 5.0f;	 // control the random displacement of each rock, choose a rational range to minimize rock collisions.

int main()
{
#ifdef DEBUG
	Timer timer;
	timer.start();
#endif 

	// shared pointer holding camera object. plane_near to 0.1f and plane_far to 100.0f by deault.
	// Camera initial position:(0.0f, 0.0f, 100.0f), initial direction: -z
	std::shared_ptr<Camera> camera = std::make_shared<Camera>(0.0f, 5.0f, 75.0f);

	// Global scene manager holding window and camera object instance with utility functions.
	// Notice: glfw and glw3 init in SceneManager constructor.
	// -------------------------------------------------------
	SceneManager scene_manager(SCR_WIDTH, SCR_HEIGHT, "bezier_curve", camera);

	// OpenGL global configs
    // ---------------------
	scene_manager.Enable(GL_DEPTH_TEST);

	// Load model(s)
	Model rock("res/models/rock/rock.obj");
	Model mars("res/models/planet/planet.obj");

	// Build & compile shader(s)
	Shader marsShader("res/shaders/instancing_mars.vs", "res/shaders/instancing_mars.fs");
	Shader rockShader("res/shaders/instancing_rock.vs", "res/shaders/instancing_rock.fs");

	// Generate a large list of semi-random model transformation matrices
	glm::mat4* modelMatrices = new glm::mat4[amount]; // the actual data storing the model matrices
	glm::vec3* axis = new glm::vec3[amount]; // Stores the rotation axis for each rock instance

	std::random_device rd;
	std::mt19937 gen(rd()); // Initialize Mersenne Twister random number generator
	std::uniform_real_distribution<float> dis(-1.0, 1.0);
	std::uniform_real_distribution<float> scaleDis(0.05, 0.2);
	std::uniform_real_distribution<float> angleDis(0.0, 360.0);
	std::uniform_real_distribution<float> axisDis(0.0, 1.0);

	// Loop to initialize each rock's model matrix
	// Notice: reversed order here
	for (size_t i = 0; i < amount; i++) {
		glm::mat4 model = glm::mat4(1.0f);

		// 3. Translation: Displace each rock along a circle with a radius and a random offset
		float angle = (float)(i) / (float)(amount) * 360.0f;
		float x = sin(angle) * radius + dis(gen) * offset;
		float y = 0.6 * dis(gen) * offset;
		float z = cos(angle) * radius + dis(gen) * offset;
		model = glm::translate(model, glm::vec3(+x, +y, +z));

		// 2. Scaling: Randomly scale each rock
		float scale = scaleDis(gen);
		model = glm::scale(model, glm::vec3(scale));

		// 1. Rotation: Apply a random initial rotation around a random axis
		float rotAngle = angleDis(gen);
		glm::vec3 randomAxis(axisDis(gen), axisDis(gen), axisDis(gen));
		model = glm::rotate(model, glm::radians(rotAngle), randomAxis);
		axis[i] = randomAxis; // Store this axis for later use

		// 4. Store the model matrix
		modelMatrices[i] = model;
	}

	// Generate a random rotation speed array for rocks
	float* rotationSpeeds = new float[amount];
	std::uniform_real_distribution<float>angleDistribution(4.0f, 8.0f);
	for (size_t i = 0; i < amount; i++)
		rotationSpeeds[i] = angleDistribution(gen);  // Random rotation speed

	// configure instanced array
	unsigned int instancingBuffer;
	glGenBuffers(1, &instancingBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, instancingBuffer);
	glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0][0], GL_STATIC_DRAW);

	// Loop through each mesh in the rock model
	for (unsigned int i = 0; i < rock.GetMesh().size(); i++) {
		// Bind the VAO of the current mesh
		unsigned int VAO = rock.GetMesh()[i].GetVAO();
		glBindVertexArray(VAO);

		// Enable and set vertex attributes for the model matrix.
		// A mat4 is treated as an array of 4 vec4s.
		for (int j = 0; j < 4; j++) {
			// Enable the vertex attribute at location 3 + j
			glEnableVertexAttribArray(3 + j);

			// Specify how the data for that attribute is retrieved from the array
			glVertexAttribPointer(3 + j, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * j));

			// Set the attribute divisor to 1 to update the attribute once per instance
			glVertexAttribDivisor(3 + j, 1);
		}

		glBindVertexArray(0);
	}

#ifdef DEBUG
	timer.stop();
#endif 

	while (!glfwWindowShouldClose(scene_manager.GetWindow())) {
		scene_manager.UpdateDeltaTime();
		scene_manager.ProcessInput();

		// Render
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Update the rotation of each rock around its own random axis at a random speed.
		// Using deltaTime to ensure frame-rate independent rotation
		for (unsigned int i = 0; i < amount; i++) {
			glm::mat4 model = modelMatrices[i];

			// Rotate around the rock's own axis
			float angle = rotationSpeeds[i] * scene_manager.GetDeltaTime();
			model = glm::rotate(model, angle, axis[i]); // random angle & random axis
			modelMatrices[i] = model;
		}
		// Update the buffer with the new transformations
		glBindBuffer(GL_ARRAY_BUFFER, instancingBuffer);
		glBufferSubData(GL_ARRAY_BUFFER, 0, amount * sizeof(glm::mat4), &modelMatrices[0][0]);

		// Configure transformation matrices
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
		glm::mat4 view = camera->GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);

		// Draw planet(mars)
		marsShader.Bind();
		marsShader.SetMat4("projection", projection);
		marsShader.SetMat4("view", view);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(4.0f));
		marsShader.SetMat4("model", model);
		mars.Render(marsShader);

		// Draw amount of rocks
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
	delete[] axis;
	delete[] rotationSpeeds;
	glfwTerminate();
}