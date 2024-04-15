// Vertex has vec3 pos, vec3 normal, vec3 texCoords, no need for tangent & bitangent in this case.
// Texture will specify type, holding id
// Optimize initialization by RVO with move semantics
// Please do care the texture name in glsl code!
// Author: Zhenhuan
// Date: 2024/4/2

#pragma once

#include <vector>
#include <string>

#include <GL/gl3w.h>

#include "shader.h"

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
};

struct Texture
{
	std::string type; // e.g., texture_diffuse, texture_specular
	unsigned int id;  // the texture id holding by opengl
	std::string filepath; 
};

class Mesh
{
public:
	Mesh() = delete;  // Deleted default constructor
	Mesh(const std::vector<Vertex>& vertices,
		const std::vector<unsigned int>& indices,
		const std::vector<Texture>& textures);  // Parameterized constructor
	~Mesh();  // Destructor

	// Move Semantics
	Mesh(Mesh&& other) noexcept;  // Move constructor
	Mesh& operator=(Mesh&& other) noexcept;  // Move assignment operator

	// Deleted Copy Semantics
	// Prevent copying as this class manages OpenGL resources
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	// Draws the mesh using the provided shader.
	// 
	// Usage:
	//   - To draw the mesh using only specific types of textures:
	//       mesh.Draw(shader, {"texture_diffuse", "texture_specular"});
	//   - To draw the mesh using all available textures:
	//       mesh.Draw(shader);
	//
	// Reminder for GLSL code:
	//   - The uniform sampler2D variables in the GLSL code should be named according
	//     to the following format: "texture_diffuseN" or "texture_specularN", where
	//     N is the texture number starting from 1.
	//
	void Render(Shader& shader, const std::vector<std::string>& textureTypesToUse = {}) const;

	// Accessors
	const unsigned int GetVAO() const { return VAO; }

public:
	// Public Members
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

private:
	void SetupMesh();  // Initialize OpenGL objects

private:
	unsigned int VAO = 0, VBO = 0, IBO = 0;
};

Mesh::Mesh(const std::vector<Vertex>& _vertices,
	const std::vector<unsigned int>& _indices,
	const std::vector<Texture>& _textures)
{
	this->vertices = _vertices;
	this->indices = _indices;
	this->textures = _textures;

	SetupMesh();
}

Mesh::~Mesh()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &IBO);
}

Mesh::Mesh(Mesh&& other) noexcept
	: VAO(other.VAO), VBO(other.VBO), IBO(other.IBO),
	vertices(std::move(other.vertices)), 
	indices(std::move(other.indices)),
	textures(std::move(other.textures))
{
	// Invalidate the moved-from object's OpenGL handles
	other.VAO = 0;
	other.VBO = 0;
	other.IBO = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
	// prevent duplication
	if (this != &other) {
		// Release any resources held by *this, otherwise memory leak possible
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &IBO);

		// Steal the resources from other
		VAO = other.VAO;
		VBO = other.VBO;
		IBO = other.IBO;
		vertices = std::move(other.vertices);
		indices = std::move(other.indices);
		textures = std::move(other.textures);

		// Invalidate the moved-from object's OpenGL handles
		other.VAO = 0;
		other.VBO = 0;
		other.IBO = 0;
	}
	return *this;
}

void Mesh::SetupMesh()
{
	if (this->VAO == 0) {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &IBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		// vertex positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

		glBindVertexArray(0);
	}
	else {
#ifdef _DEBUG
		std::cout << "failed to setup mesh!\n";
#endif
	}
}

void Mesh::Render(Shader& shader, const std::vector<std::string>& textureTypesToUse) const
{
	// Start from material.diffuse1 or material.specular1
	size_t diffuseNr = 1, specularNr = 1, normalNr = 1, heightNr = 1;

	for (int i = 0; i < textures.size(); i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		// Get texture number£¨N in diffuse_textureN £©
		std::string name = textures[i].type;

		// Skip this texture if it's not in the list of types to use
		if (!textureTypesToUse.empty() &&
			std::find(textureTypesToUse.begin(), textureTypesToUse.end(), name) == textureTypesToUse.end()) {
			continue;
		}

		std::string number;
		if (name == "texture_diffuse")
			number = std::to_string(diffuseNr++);
		else if (name == "texture_specular")
			number = std::to_string(specularNr++);
		else if (name == "texture_normal")
			number = std::to_string(normalNr++);
		else if (name == "texture_height")
			number = std::to_string(heightNr++);

		// can change this line based on the specific shader code
		shader.SetInt((name + number).c_str(), i);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
	}

	// Draw mesh
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}