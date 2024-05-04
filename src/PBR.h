#pragma once
#ifndef PBR_H
#define PBR_H

#include "config.h"
#include "geometry_renderers.h"
#include "model.h"
#include "scene_manager.h"
#include "shader.h"
#include <glm/glm.hpp>

void LoadPBRMaterials(unsigned int& albedo, unsigned int& normal, unsigned int& metallic, unsigned int& roughness, unsigned int& ao)
{
	albedo = LoadTexture("res/textures/pbr/rusted_iron/albedo.png");
	normal = LoadTexture("res/textures/pbr/rusted_iron/normal.png");
	metallic = LoadTexture("res/textures/pbr/rusted_iron/metallic.png");
	roughness = LoadTexture("res/textures/pbr/rusted_iron/roughness.png");
	ao = LoadTexture("res/textures/pbr/rusted_iron/ao.png");
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