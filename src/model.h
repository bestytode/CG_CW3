#pragma once

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif 

#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/gl3w.h>

#include "mesh.h"
#include "shader.h"

// Utility function for loading a 2D texture from file
// Loading HDR texture when isHDR is true(by default isHDR is true)
// Marked as static to avoid potential collision
static unsigned int LoadTexture(const std::string& path, bool isHDR = true);

class Model
{
public:
	Model(const std::string& objFilePath) {
		LoadOBJ(objFilePath);
	}

	 // Draws the model using the provided shader.
	 // Usage:
	 //  - To draw the model using only specific types of textures:
	 //      model.draw(shader, {"texture_diffuse", "texture_specular"});
	 //  - To draw the model using all available textures:
	 //      model.draw(shader);
	void Render(Shader& shader, const std::vector<std::string>& textureTypeToUse = {}) {
		shader.Bind();
		for (unsigned int i = 0; i < meshes->size(); i++) {
			(*meshes)[i].Render(shader, textureTypeToUse);
		}
		shader.Unbind();
	}
	
public:
	int GetMeshNumbers(const std::string& path);
	void LoadOBJ(const std::string& objPath);
	std::unordered_map<std::string, std::vector<Texture>> LoadMTL(const std::string& mtlFilePath);

private:
	std::vector<Mesh>* meshes;
};

// Optional use: helper function to trim leading and trailing whitespaces from a string
std::string Trim(const std::string& str) 
{
	std::string s = str;

	// Left trim
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
		}));

	// Right trim
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
		}).base(), s.end());

	return s;
}

int Model::GetMeshNumbers(const std::string& path) {
	std::ifstream file(path);
	std::string line;
	int count = 0;

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + path);
	}

	while (std::getline(file, line)) {
		// Trim leading whitespace
		//line = Trim(line);
		
		// Count lines that strictly start with 'o ' (indicating a new object)
		if (line.substr(0, 2) == "o ") {
			count++;
		}
	}
	file.close();
	return count;
}

void Model::LoadOBJ(const std::string& objFilePath)
{
	std::ifstream file(objFilePath);
	if (!file.is_open()) {
		throw std::runtime_error("Could not open OBJ file: " + objFilePath);
	}

	std::string line;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	std::unordered_map<std::string, std::vector<Texture>> materialTextures = LoadMTL("path_to_mtl_file");

	while (getline(file, line)) {
		std::stringstream ss(line);
		std::string prefix;
		ss >> prefix;
		if (prefix == "o") {
			if (!vertices.empty()) {
				meshes->emplace_back(vertices, indices, textures);
				vertices.clear();
				indices.clear();
				textures.clear();
			}
			continue;
		}
		else if (prefix == "v") {
			// Parse vertex
		}
		else if (prefix == "vt") {
			// Parse texture coordinate
		}
		else if (prefix == "vn") {
			// Parse normal
		}
		else if (prefix == "usemtl") {
			std::string materialName;
			ss >> materialName;
			// Assuming one material per mesh for simplicity
			textures = materialTextures[materialName];
		}
		else if (prefix == "f") {
			// Parse face
		}
	}

	// Add the last mesh
	if (!vertices.empty()) {
		meshes->emplace_back(vertices, indices, textures);
	}
}

std::unordered_map<std::string, std::vector<Texture>> Model::LoadMTL(const std::string& mtlFilePath) 
{
	std::unordered_map<std::string, std::vector<Texture>> materialTextures;
	std::ifstream file(mtlFilePath);
	std::string line, key, materialName;
	Texture texture;

	while (getline(file, line)) {
		std::stringstream ss(line);
		ss >> key;
		if (key == "newmtl") {
			if (!materialName.empty()) {
				materialTextures[materialName].push_back(texture);
			}
			ss >> materialName;
			texture = Texture();
		}
		else if (key == "map_Kd" || key == "map_Ka" || key == "map_Ks") {
			std::string filepath;
			ss >> filepath;
			texture.filepath = filepath;
			texture.id = LoadTexture(filepath);
			texture.type = key;
		}
	}
	if (!materialName.empty()) {
		materialTextures[materialName].push_back(texture);
	}
	return materialTextures;
}

static unsigned int LoadTexture(const std::string& path, bool isHDR)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	void* data;

	// Using stbi_load or stbi_loadf for different texture type
	if (!isHDR) {
		data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
	}
	else {
		stbi_set_flip_vertically_on_load(true);
		data = stbi_loadf(path.c_str(), &width, &height, &nrComponents, 0);
	}

	if (data) {
		GLenum format;
		GLenum dataType = (isHDR) ? GL_FLOAT : GL_UNSIGNED_BYTE; // Set GL_FLOAT or GL_UNSIGNED_BYTE for different texture data type
		GLint internalFormat; // Using 16-bit float format for HDR, otherwise internalFormat equals to format

		if (nrComponents == 1) format = internalFormat = GL_RED;
		else if (nrComponents == 3) format = internalFormat = GL_RGB;
		else if (nrComponents == 4) format = internalFormat = GL_RGBA;
		else std::cerr << "Invalid texture format: Unsupported number of components!\n";

		if (isHDR) internalFormat = GL_RGB16F;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, data);
		if (!isHDR)
			glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (!isHDR)
			stbi_image_free(data);
		else
			stbi_image_free((float*)data);
	}
	else {
		std::cerr << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
		return -1;
	}

	return textureID;
}
