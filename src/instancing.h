#pragma once
#ifndef INSTANCING_H
#define INSTANCING_H

#include "config.h"
#include <glm/glm.hpp>
#include <random>

void InitModelMatricesAndRotationSpeeds(glm::mat4* modelMatrices, glm::vec3* rotationAxis, float* rotationSpeeds)
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

void SetupInstancingBuffer(unsigned int& instancingBuffer, glm::mat4* modelMatrices, const Model& rock)
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

void UpdateModelMatrices(float deltaTime)
{
	for (unsigned int i = 0; i < amount; i++) {
		glm::mat4 model = modelMatrices[i];

		// Rotate around the rock's own axis
		float angle = 0.5f * rotationSpeeds[i] * deltaTime;
		model = glm::rotate(model, angle, rotationAxis[i]); // random angle & random axis
		modelMatrices[i] = model;
	}
	// Update the buffer with the new transformations
	glBindBuffer(GL_ARRAY_BUFFER, instancingBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, amount * sizeof(glm::mat4), &modelMatrices[0][0]);
}

void RenderInstancingRocks(Shader& rockShader, Model& rock)
{
	rockShader.Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, rock.GetMesh()[0].textures[0].id);

	// Special case: The rock model has only one mesh.
	// Directly bind its VAO and draw it instanced.
	// Note: This won't work for models with multiple meshes.
	glBindVertexArray(rock.GetMesh()[0].GetVAO());
	glDrawElementsInstanced(GL_TRIANGLES, (unsigned int)(rock.GetMesh()[0].indices.size()), GL_UNSIGNED_INT, 0, amount);
	glBindVertexArray(0);
}

#endif // !INSTANCING_H