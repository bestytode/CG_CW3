// SSAO simle implementation:
// 
// 1. Note that the depth values being compared are in view space.
// 
// 2. Various techniques are employed to improve quality and reduce sample size, 
//    such as scaling samples in the kernel, tiling a 4x4 noise texture, applying a blur process, 
//    and using the smoothstep function for range checking.
// 
// 3. The sample kernel is distributed in a hemisphere and is directly used in the GLSL shader.
// 
// 4. The noise texture is aligned with the z-axis, consistent with the sample kernel, 
//    and is combined with a TBN matrix for orientation.
// 
// 5. The sample[i] is used to sample from the off-screen rendered texture after converting 
//    from view space to the [0, 1] range.
// 
// 6. Only a single float value (GL_RED channel) is stored as the output color buffer texture 
//    for both the SSAO and blur shaders, as we only need an occlusion coefficient (i.e., out float FragColor).
// 
// 2023-10-7
// Author: Yu

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

// SSAO settings
static bool enableSSAO = true;
static unsigned int sampleSize = 16;
static float radius = 1.0f;

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

	// build and compile shader(s)
	// ---------------------------
	Shader shaderGeometryPass("res/shaders/ssao_geometry.vs", "res/shaders/ssao_geometry.fs");
	Shader shaderSSAO("res/shaders/ssao.vs", "res/shaders/ssao.fs");
	Shader shaderSSAOBlur("res/shaders/ssao.vs", "res/shaders/ssao_blur.fs");
	Shader shaderLightingPass("res/shaders/ssao.vs", "res/shaders/ssao_lighting.fs");
	Shader shaderLightSource("res/shaders/deferred_light_box.vs", "res/shaders/deferred_light_box.fs");

	// load model(s)
	// -------------
	Model nanosuit("res/models/nanosuit/nanosuit.obj");
	//Model backpack("res/models/backpack/backpack.obj");

	// Set VAO for geometry shape for later use
	yzh::Quad quad;
	yzh::Cube cube;
	yzh::Sphere sphere;

	// Set up G-Buffer with 3 textures:
	// 
	// 1. Position
	// 2. Color
	// 3. Normal 
	// ---------------
	unsigned int gBuffer;
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	unsigned int gPosition, gNormal, gAlbedo;
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	glGenTextures(1, &gAlbedo);
	glBindTexture(GL_TEXTURE_2D, gAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);

	unsigned int attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindBuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "GBuffer Framebuffer not complete!" << std::endl;

	// 2. Create SSAO texture
	// ----------------------
	unsigned int ssaoFBO, ssaoColorBuffer;
	glGenFramebuffers(1, &ssaoFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

	glGenTextures(1, &ssaoColorBuffer);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSAO Framebuffer not complete!" << std::endl;

	// 3. Blur SSAO texture to remove noise
	// ------------------------------------
	unsigned int ssaoBlurFBO, ssaoColorBufferBlur;
	glGenFramebuffers(1, &ssaoBlurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);

	glGenTextures(1, &ssaoColorBufferBlur);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 4. Sample kernel (in tangent space)
	// -----------------------------------
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(-1, 1);

	std::vector<glm::vec3>sampleKernel;

	for (size_t i = 0; i < sampleSize; i++) {
		glm::vec3 sample(dis(gen), dis(gen), (dis(gen) * 0.5f + 0.5f)); // Generate random sample point in a hemisphere oriented along the z-axis
		sample = glm::normalize(sample);
		sample *= dis(gen); // Provide random length of sample vector

		// Scale samples so that they are more aligned to the center of the kernel
		float scale = float(i) / (float)sampleSize;
		scale = glm::mix(0.1f, 1.0f, scale * scale);
		sample *= scale;
		sampleKernel.emplace_back(sample);
	}

	// 5. Noise texture (in tangent space)
	unsigned int noiseTexture;
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);

	std::vector<glm::vec3>ssaoNoise;
	for (size_t i = 0; i < 16; i++) {
		// Set z-coordinate to 0.0 to ensure the points will still be distributed in a hemisphere oriented along z-axis,
		glm::vec3 noise(dis(gen), dis(gen), 0.0f);
		ssaoNoise.emplace_back(noise);
	}
	// Notice we set both width and height to 4, which is the parameter that related to the size of 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // Use GL_REPEAT to fill the quad as the noise texture is relatively small
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// lighting info
	// -------------
	struct Light {
		glm::vec3 position = glm::vec3(2.0, 4.0, -2.0);
		glm::vec3 color = glm::vec3(0.2, 0.2, 0.7);
		const float linear = 0.09f;
		const float quadratic = 0.032f;
	};
	Light light;

	// shader configs
	// --------------
	shaderLightingPass.Bind();
	shaderLightingPass.SetInt("gPosition", 0);
	shaderLightingPass.SetInt("gNormal", 1);
	shaderLightingPass.SetInt("gAlbedo", 2);
	shaderLightingPass.SetInt("ssao", 3);
	shaderSSAO.Bind();
	shaderSSAO.SetInt("gPosition", 0);
	shaderSSAO.SetInt("gNormal", 1);
	shaderSSAO.SetInt("noiseTexture", 2);
	shaderSSAOBlur.Bind();
	shaderSSAOBlur.SetInt("ssaoInput", 0);

#ifdef DEBUG
	timer.stop();
#endif 

	while (!glfwWindowShouldClose(scene_manager.GetWindow())) {
		scene_manager.UpdateDeltaTime();
		scene_manager.ProcessInput();

		// Render
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// press space to switch state
		if (glfwGetKey(scene_manager.GetWindow(), GLFW_KEY_SPACE) == GLFW_RELEASE) {
			enableSSAO != enableSSAO;
		}

		// 1. Geometry Pass: render scene's geometry/color data into gbuffer
		// -----------------------------------------------------------------
		//glEnable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shaderGeometryPass.Bind();

		glm::mat4 projection = glm::perspective(glm::radians(camera->fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera->GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0, -1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(20.0f, 1.0f, 20.0f));
		shaderGeometryPass.SetMat4("projection", projection);
		shaderGeometryPass.SetMat4("view", view);
		shaderGeometryPass.SetMat4("model", model);
		shaderGeometryPass.SetInt("invertedNormals", 1); // invert normals as we're inside the cube
		cube.Render();
		shaderGeometryPass.SetInt("invertedNormals", 0);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
		model = glm::scale(model, glm::vec3(1.0f));
		shaderGeometryPass.SetMat4("model", model);
		nanosuit.Render(shaderGeometryPass);
		//backpack.Render(shaderGeometryPass);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 2. Perform SSAO Calculation
		// ---------------------------
		//glDisable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		shaderSSAO.Bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		for (size_t i = 0; i < sampleSize; i++) {
			shaderSSAO.SetVec3("samples[" + std::to_string(i) + "]", sampleKernel[i]);
		}
		shaderSSAO.SetMat4("projection", projection);
		shaderSSAO.SetFloat("kernelSize", (float)sampleSize);
		shaderSSAO.SetFloat("radius", radius);
		quad.Render();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 3. Blur ssao texture to remove noise
		// ------------------------------------
		//glDisable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		shaderSSAOBlur.Bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
		quad.Render();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 4. Lighting Pass: traditional deferred Blinn-Phong lighting now with added screen-space ambient occlusion
		// ---------------------------------------------------------------------------------------------------------
		//glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shaderLightingPass.Bind();

		// Notice we multiply view matrix here instead of in glsl code
		glm::vec3 lightPosView = glm::vec3(camera->GetViewMatrix() * glm::vec4(light.position, 1.0));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gAlbedo);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);

		// Send lighting uniforms
		shaderLightingPass.SetVec3("light.Position", lightPosView);
		shaderLightingPass.SetVec3("light.Color", light.color);
		shaderLightingPass.SetFloat("light.Linear", light.linear);
		shaderLightingPass.SetFloat("light.Quadratic", light.quadratic);

		shaderLightingPass.SetInt("enableSSAO", enableSSAO); 
		quad.Render();

		// 5. Render light source
		// ----------------------
		glEnable(GL_DEPTH_TEST);
		shaderLightSource.Bind();

		model = glm::mat4(1.0f);
		model = glm::translate(model, light.position);
		model = glm::scale(model, glm::vec3(0.05f));

		shaderLightSource.SetMat4("projection", projection);
		shaderLightSource.SetMat4("view", view);
		shaderLightSource.SetMat4("model", model);
		shaderLightSource.SetVec3("lightColor", light.color);
		sphere.Render();

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(scene_manager.GetWindow());
		glfwPollEvents();
	}

	// Optional: release all the resources of OpenGL (VAO, VBO, etc.)
	glfwTerminate();
}