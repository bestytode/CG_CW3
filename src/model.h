#pragma once
#ifndef MODEL_H
#define MODEL_H

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif 

#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include <tuple>
#include <map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/gl3w.h>

#include "mesh.h"
#include "shader.h"

/**
 * Loads a texture from a file and returns its OpenGL texture ID.
 * Assumes an OpenGL context is available and properly set up.
 *
 * @param textureFilepath Path to the texture file.
 * @return OpenGL texture ID for the loaded texture.
 */
static unsigned int LoadTexture(const std::string& path, bool isHDR = false);

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
		for (unsigned int i = 0; i < meshes.size(); i++) 
			meshes[i].Render(shader, textureTypeToUse);
	}
	
	std::vector<Mesh>& GetMesh() { return this->meshes; }
	const std::vector<Mesh>& GetMesh() const { return this->meshes; }
private:
	/**
	 * Loads an OBJ file and constructs meshes from it.
	 * Each 'o' line in the OBJ file starts a new mesh.
	 * Faces within an object are grouped into a single mesh.
	 *
	 * @param objFilePath Path to the OBJ file.
	 */
	void LoadOBJ(const std::string& objPath);

	/**
	 * Loads material properties from an MTL file and maps them to texture objects.
	 * This mapping aids in assigning materials to faces in the OBJ file.
	 *
	 * @param mtlFilePath Path to the MTL file.
	 * @return A map from material names to their corresponding textures.
	 */
	std::unordered_map<std::string, std::vector<Texture>> LoadMTL(const std::string& mtlFilePath);

private:
	//std::vector<Mesh>* meshes;
	std::vector<Mesh>meshes;;
};

void Model::LoadOBJ(const std::string& objFilePath)
{
	std::string baseName = objFilePath.substr(0, objFilePath.find_last_of("."));
	std::string mtlFilePath = baseName + ".mtl";

	std::unordered_map<std::string, std::vector<Texture>> materialTextures = LoadMTL(mtlFilePath);

	std::ifstream file(objFilePath);
	if (!file.is_open()) {
		throw std::runtime_error("Could not open OBJ file: " + objFilePath);
	}

	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texCoords;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	std::string line;
	while (getline(file, line)) {
		std::stringstream ss(line);
		std::string prefix;
		ss >> prefix;
		if (prefix == "o") {
			if (!vertices.empty()) {
				meshes.emplace_back(Mesh(vertices, indices, textures));
				vertices.clear();
				indices.clear();
				textures.clear();
			}
		}
		else if (prefix == "v") {
			glm::vec3 position;
			ss >> position.x >> position.y >> position.z;
			positions.push_back(position);
		}
		else if (prefix == "vt") {
			glm::vec2 texCoord;
			ss >> texCoord.x >> texCoord.y;
			texCoords.emplace_back(texCoord.x, 1.0f - texCoord.y); // Flip the v coordinate
		}
		else if (prefix == "vn") {
			glm::vec3 normal;
			ss >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (prefix == "usemtl") {
			std::string materialName;
			ss >> materialName;
			textures = materialTextures[materialName];
		}
		else if (prefix == "f") {
			for (int i = 0; i < 3; i++) {
				unsigned int vIdx, vtIdx, vnIdx;
				char slash;
				ss >> vIdx >> slash >> vtIdx >> slash >> vnIdx;
				vIdx--; vtIdx--; vnIdx--;
				Vertex vertex = {
					positions[vIdx], // Position
					normals[vnIdx], // Normal
					texCoords[vtIdx] // Texture Coordinate
				};
				vertices.push_back(vertex);
				indices.push_back(static_cast<unsigned int>(vertices.size() - 1));
			}
		}
	}

	if (!vertices.empty()) {
		meshes.emplace_back(Mesh(vertices, indices, textures));
	}

#ifdef _DEBUG
	std::cout << "Load model success!\n";
#endif
}

std::unordered_map<std::string, std::vector<Texture>> Model::LoadMTL(const std::string& mtlFilePath) 
{
	std::unordered_map<std::string, std::vector<Texture>> materials;
	std::ifstream file(mtlFilePath);
	std::string line;
	std::string currentMaterialName;
	std::vector<Texture> currentTextures;

	// Extract directory path from MTL file path
	std::string directory = mtlFilePath.substr(0, mtlFilePath.find_last_of("\\/") + 1);

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open MTL file: " + mtlFilePath);
	}

	while (getline(file, line)) {
		std::stringstream ss(line);
		std::string key;
		ss >> key;
		if (key == "newmtl") {
			if (!currentMaterialName.empty()) {
				materials[currentMaterialName] = currentTextures; // Store previous material's textures
				currentTextures.clear();
			}
			ss >> currentMaterialName; // Update current material name
		}
		else if (key == "map_Kd" || key == "map_Ks" || key == "map_Ka" || key == "map_Bump") {
			Texture texture;
			std::string filepath;
			ss >> filepath;
			// Construct full path by appending directory to the texture filename
			filepath = directory + filepath;
            // Map the MTL file keys to standard texture types
            if (key == "map_Ka") {
                texture.type = "texture_ambient";
            } else if (key == "map_Kd") {
                texture.type = "texture_diffuse";
            } else if (key == "map_Ks") {
                texture.type = "texture_specular";
			} else if (key == "map_Bump") {
				texture.type = "texture_height";
			}
			texture.filepath = filepath;
			texture.id = LoadTexture(filepath); // Load texture and get ID
			currentTextures.push_back(texture);
		}
	}

	if (!currentMaterialName.empty()) {
		materials[currentMaterialName] = currentTextures; // Store the last material's textures
	}

	return materials;
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

#endif // !MODEL_H