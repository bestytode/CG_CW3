#pragma once
#ifndef PBR_H
#define PBR_H

#include "config.h"
#include "geometry_renderers.h"
#include "model.h"
#include "scene_manager.h"
#include "shader.h"
#include <glm/glm.hpp>

static unsigned int LoadPBRTexture(const std::string& path, bool isHDR = false)
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

void LoadPBRMaterials(unsigned int& albedo, unsigned int& normal, unsigned int& metallic, unsigned int& roughness, unsigned int& ao)
{
	albedo = LoadPBRTexture("res/textures/pbr/rusted_iron/albedo.png");
	normal = LoadPBRTexture("res/textures/pbr/rusted_iron/normal.png");
	metallic = LoadPBRTexture("res/textures/pbr/rusted_iron/metallic.png");
	roughness = LoadPBRTexture("res/textures/pbr/rusted_iron/roughness.png");
	ao = LoadPBRTexture("res/textures/pbr/rusted_iron/ao.png");
}

void RenderPBRMars(Shader& pbrShader, yzh::Sphere& sphere)
{
	pbrShader.Bind();

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

	sphere.Render();
}

#endif // PBR_H