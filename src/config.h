// Notice:
// global variables are dangerous, which could result in naming collisions.
// Mark them as static or in namespace scope would be better.
// For convinience, they are used in this assessment.

#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#include <glm/glm.hpp>

constexpr float PI = 3.14159265358979323846f;

// Screen
// ------
constexpr int SCR_WIDTH = 1920;  // Screen width
constexpr int SCR_HEIGHT = 1080; // Screen height

// Camera
// ------
constexpr float z_near = 0.1f;   // camera near
constexpr float z_far = 1000.0f; // camera far

// Rock instancing
// ---------------
unsigned int instancingBuffer = 0; // instancing buffer id
const unsigned int amount = 1000; // total amount of rocks

const float radius = 50.0f; // belt radius around mars
const float offset = 3.0f;	 // control the random displacement of each rock, choose a rational range to minimize rock collisions.
const float asteroidScale = 2.0f;	 // control the random displacement of each rock, choose a rational range to minimize rock collisions.

glm::mat4* modelMatrices = new glm::mat4[amount]; // model matrices for rocks
glm::vec3* rotationAxis = new glm::vec3[amount];  // rotation axis for rocks
float* rotationSpeeds = new float[amount];        // rotation speed for rocks

// PBR materials
// -------------
unsigned int albedo = 0;     // albedo texture id
unsigned int normal = 0;     // normal texture id
unsigned int metallic = 0;   // metallic texture id
unsigned int roughness = 0;  // roughness texture id
unsigned int ao = 0;         // ao texture id

float metallicScale = 1.0f;  // Scale factor for metallic
float roughnessScale = 1.0f; // Scale factor for roughness
glm::vec3 albedoScale(1.0f, 1.0f, 1.0f); // Scale factor for albedo

// Lighting infos
// --------------
glm::vec3 lightPosition = glm::vec3(0.0f, 0.0f, 12.0f);
glm::vec3 lightColor = glm::vec3(150.0f, 150.0f, 150.0f);

// Sky box infos
// -------------
unsigned int cubemapTexture = 0;
unsigned int skyboxVAO, skyboxVBO;

#endif // CONFIG_H