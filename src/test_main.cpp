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

constexpr int SCR_WIDTH = 1920;  // Screen width
constexpr int SCR_HEIGHT = 1080; // Screen height

static glm::vec3 calculateBezierPoint(float t, const std::vector<glm::vec3>& controlPoints)
{
	float u = 1 - t;
	float tt = t * t;
	float uu = u * u;
	float uuu = uu * u;
	float ttt = tt * t;

	glm::vec3 p = uuu * controlPoints[0]; // first term
	p += 3 * uu * t * controlPoints[1]; // second term
	p += 3 * u * tt * controlPoints[2]; // third term
	p += ttt * controlPoints[3]; // fourth term

	return p;
}

int main()
{
	// shared pointer holding camera object. plane_near to 0.1f and plane_far to 100.0f by deault.
	std::shared_ptr<Camera> camera = std::make_shared<Camera>(0.0f, 0.0f, 3.0f);

	// Global scene manager holding window and camera object instance with utility functions.
	// Notice: glfw and glw3 init in SceneManager constructor.
	// -------------------------------------------------------
	SceneManager scene_manager(SCR_WIDTH, SCR_HEIGHT, "hnzz", camera);

	// OpenGL global configs
	// ---------------------
	scene_manager.Enable(GL_DEPTH_TEST);
	scene_manager.Enable(GL_MULTISAMPLE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	scene_manager.Enable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	std::vector<glm::vec3> controlPoints(4); // Fixed array of 4 glm::vec3 for control points
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(-5.0, 5.0);

	for (auto& point : controlPoints) {
		point.x = dis(gen);
		point.y = dis(gen);
		point.z = 0.0f; // Fixed at 0.0
	}
	std::vector<glm::vec3> bezierCurvePoints;
	for (float t = 0; t <= 1.0f; t += 0.01f) { // Adjust step size as needed
		bezierCurvePoints.push_back(calculateBezierPoint(t, controlPoints));
	}

	GLuint VBO, VAO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);

	// Bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, bezierCurvePoints.size() * sizeof(glm::vec3), &bezierCurvePoints[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	unsigned int pointsVAO, pointsVBO;
	glGenBuffers(1, &pointsVBO);
	glGenVertexArrays(1, &pointsVAO);
	glBindVertexArray(pointsVAO);
	glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
	glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(glm::vec3), &controlPoints[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);


	// shader configs
	Shader shader("res/shaders/debug_light.vs", "res/shaders/debug_light.fs");

	// Imgui configs
	// ----------------
	float windowWidth = 480.0f, windowHeight = 1080.0f;
	float windowPosX = 0.0f, windowPosY = 0.0f;
	float fontSizeScale = 0.7f;

	//timer.stop(); // Timer stops

	bool firstTimeOutputPosition = true;




	Model model = Model("ass");
	//int a = model.GetMeshNumbers("res/models/nanosuit/nanosuit.obj");
	int a = model.GetMeshNumbers("res/models/backpack/backpack.obj");

	// Render loop
	while (!glfwWindowShouldClose(scene_manager.GetWindow())) {
		scene_manager.UpdateDeltaTime();
		scene_manager.ProcessInput();

		// Render
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// test rendering cones
		shader.Bind();
		glm::mat4 projection = glm::perspective(glm::radians(camera->fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera->GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::scale(model, glm::vec3(0.5f));
		shader.SetMat4("projection", projection);
		shader.SetMat4("view", view);
		shader.SetMat4("model", model);
		shader.SetInt("use_orange_color", 1);
		shader.SetInt("use_red_color", 0);
		glBindVertexArray(VAO);
		glDrawArrays(GL_LINE_STRIP, 0, bezierCurvePoints.size());
		glBindVertexArray(0);

		glPointSize(10.0f);
		shader.SetInt("use_orange_color", 0);
		shader.SetInt("use_red_color", 1);
		glBindVertexArray(pointsVAO);
		glDrawArrays(GL_POINTS, 0, 4);
		glBindVertexArray(0);

		if (firstTimeOutputPosition) {
			for (size_t i = 0; i < controlPoints.size(); i++) {
				std::cout << "control points: " << controlPoints[i].x << " " << controlPoints[i].y << " " << controlPoints[i].z << std::endl;
			}
			firstTimeOutputPosition = false;
		}

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(scene_manager.GetWindow());
		glfwPollEvents();
	}
}