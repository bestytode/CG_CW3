#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static const float moveSpeed = 5.0f;  // The speed at which the camera moves, influenced by keyboard inputs
static const float mouseSensitivity = 0.1f; // Sensitivity of mouse input for camera orientation
static const float minFov = 5.0f;
static const float maxFov = 45.0f;
static const float minPitch = -89.0f;
static const float maxPitch = 89.0f;
static const glm::vec3 defaultCameraPosition = glm::vec3(0.0f, 0.0f, 3.0f);
static const glm::vec3 defaultCameraDirection = glm::vec3(0.0f, 0.0f, -1.0f);
static const glm::vec3 defaultCameraWorldUp = glm::vec3(0.0f, 1.0f, 0.0f);

enum Direction
{
    FORWARD,  // W
    BACKWARD, // A
    LEFT,     // S
    RIGHT     // D
};

// The Camera class provides functionalities for a 3D camera in OpenGL.
// It handles camera movement through keyboard and mouse inputs, and 
// calculates the corresponding view matrix for use in the shader.
//
// Usage Example:
// Camera camera(0.0f, 0.0f, 3.0f);
// glm::mat4 view = camera.GetViewMatrix();
class Camera
{
public:
	Camera(float _x, float _y, float _z)
		: position(glm::vec3(_x, _y, _z)), direction(defaultCameraDirection), up(defaultCameraWorldUp)
	{
		right = glm::normalize(glm::cross(direction, worldUp));
		UpdateCameraVectors();
	}

	Camera(glm::vec3 _position = defaultCameraPosition)
		: position(_position), direction(defaultCameraDirection), up(defaultCameraWorldUp)
	{
		right = glm::normalize(glm::cross(direction, worldUp));
		UpdateCameraVectors();
	}

    // Rule of Five
    Camera(const Camera&) = default;            
    Camera& operator=(const Camera&) = default; 
    Camera(Camera&&) = default;                 
    Camera& operator=(Camera&&) = default;      

    // processes input received from any keyboard-like input system. 
    // Accepts input parameter in the form of camera defined ENUM 
    void ProcessKeyboard(Direction _direction, float deltaTime);

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrain = true);

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset);

    glm::mat4 GetViewMatrix() const{
        return glm::lookAt(position, position + direction, up);
    }

	//float GetMovementSpeed() const { return movementSpeed; }
	//float GetMouseSensitivity() const { return mouseSensitivity; }
	//void SetMovementSpeed(float newSpeed) { movementSpeed = newSpeed; }
	//void SetMouseSensitivity(float newSensitivity) { mouseSensitivity = newSensitivity; }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void UpdateCameraVectors();

public:
    // for easy of use, public variables here
    glm::vec3 position; // camera position
    float fov = 45.0f; // field of view

private:
    glm::vec3 direction;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f); // set to (0, 1, 0) by default

    // Euler Angles
    float yaw = -90.0f;   // Yaw is the angle between the positive x-axis and the projection of the point onto the xy-plane. Measured in degrees.
    float pitch = 0.0f; // Pitch is the angle between the positive z-axis and the line from the origin to the point. Measured in degrees.


};

void Camera::ProcessKeyboard(Direction _direction, float deltaTime)
{
	float velocity = moveSpeed * deltaTime;

	if (_direction == FORWARD)
		position += this->direction * velocity;
	if (_direction == BACKWARD)
		position -= this->direction * velocity;
	if (_direction == LEFT)
		position -= this->right * velocity;
	if (_direction == RIGHT)
		position += this->right * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrain)
{
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;
    yaw += xoffset;
    pitch += yoffset;

    // make sure that when pitch is out of bounds
    if (constrain) 
        pitch = std::max(minPitch, std::min(pitch, maxPitch));  // More robust clamping

    // update Front, Right and Up Vectors
    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
    fov -= yoffset;
    if (fov <= minFov)
        fov = minFov;
    if (fov >= maxFov)
        fov = maxFov;
}

void Camera::UpdateCameraVectors()
{
    // calculate the new direction vector
    glm::vec3 _direction = glm::vec3(1.0f);
    _direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    _direction.y = sin(glm::radians(pitch));
    _direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    this->direction = glm::normalize(_direction);

    // Also re-calculate the Right and Up vector
    this->right = glm::normalize(glm::cross(direction, worldUp));
    this->up = glm::normalize(glm::cross(right, direction));
}

#endif // !CAMERA_H