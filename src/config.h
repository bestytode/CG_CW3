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
const unsigned int amount = 800; // total amount of rocks

const float radius = 40.0f; // belt radius around mars
const float offset = 4.0f;	 // control the random displacement of each rock, choose a rational range to minimize rock collisions.
const float asteroidScale = 2.5f;	 // control the random displacement of each rock, choose a rational range to minimize rock collisions.

glm::mat4* modelMatrices = new glm::mat4[amount]; // model matrices for rocks
glm::vec3* rotationAxis = new glm::vec3[amount];  // rotation axis for rocks
float* rotationSpeeds = new float[amount];        // rotation speed for rocks
const float rotationSpeedScale = 0.2f;

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

bool togglePBRNormal = false;  // flag to enable geometry shader for planet
float normal_magnitude = 0.4f; // scale factor for normal
glm::vec3 normal_color = glm::vec3(1.0f, 1.0f, 0.0f); // normal color

// Sky box infos
// -------------
unsigned int cubemapTexture = 0;
unsigned int skyboxVAO = 0, skyboxVBO = 0;

// Nanosuit infos
// --------------
const float Ns = 96.0f;
const float Ka = 0.02f;
const float Kd = 0.64f;
const float Ks = 0.5f;

bool toggleNanosuitMovement = false; // notice: this will also disable camera movement when true
bool moveForward = false;
bool moveBackward = false;
bool moveLeft = false;
bool moveRight = false;
bool moveUp = false;
bool moveDown = false;
float rotationAngle = 0.0f;
float rotationDX = 0.01f; // infinitesimally small change

bool enableNanosuitExplosion = false; // press b to switch
float explosion_magnitude = 2.0f;
float maxNanosuitExplosionDuration = 5.0f; // explosion max duration
float startNanosuitExplosionTime = 0.0f;   // record start time for nanosuit explosion

// Lighting infos
// --------------
glm::vec3 lightPosition = glm::vec3(0.0f, 0.0f, 12.0f);
glm::vec3 lightColor = glm::vec3(0.0f, 0.0f, 150.0f);

glm::vec3 directionalLightDirection = glm::vec3(1.0f, -0.4f, 0.0f);
glm::vec3 directionalLightColor = glm::vec3(15.0f, 15.0f, 15.0f);
const float directionalLightScale = 0.5f;

const float curveSize = 12.0f;

glm::vec3 p0(-curveSize, 0.0, 0.0);  // Start point at left
glm::vec3 p1(-curveSize, 0.0, 1.5f * curveSize);  // Control point 1 upwards
glm::vec3 p2(curveSize, 0.0, 1.5f * curveSize);   // Control point 2 upwards
glm::vec3 p3(curveSize, 0.0, 0.0);   // End point at right

// update positional light's position, using bezier curve
glm::vec3 UpdatePositionalLight(float time)
{
	float t = (sin(time) + 1.0f) / 2.0f; // Normalize t to [0, 1]
	float u = 1.0f - t;
	float tt = t * t;
	float uu = u * u;
	float uuu = uu * u;
	float ttt = tt * t;

	glm::vec3 p = uuu * p0;
	p += 3.0f * uu * t * p1;
	p += 3.0f * u * tt * p2;
	p += ttt * p3;
	return p;
}

// update directional light direction, with fixed y of -0.4f
glm::vec3 UpdateDirectionalLight(float totalTime)
{
	float x = -cos(totalTime);
	float y = -0.4f;
	float z = -sin(totalTime);

	return glm::normalize(glm::vec3(x, y, z));
}


#endif // CONFIG_H