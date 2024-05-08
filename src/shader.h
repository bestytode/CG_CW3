#pragma once
#ifndef SHADER_H
#define SHADER_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

#include <GL/gl3w.h>
#include <glm/glm.hpp>

// The Shader class encapsulates OpenGL shader programs.
// It provides functionalities for creating, compiling, and linking shaders,
// as well as setting uniform variables.
// 
// Usage Example:
// Shader shader("vertexShaderPath", "fragmentShaderPath");
// Shader shader("vertexShaderPath", "fragmentShaderPath", "geometryShaderPath");
// 
// shader.Bind();
// shader.SetVec3("some_uniform", glm::vec3(1.0f, 0.0f, 0.0f));
// shader.Unbind();
// ------------------
class Shader
{
public:
	Shader() = delete;

	Shader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath, const std::string& geometryShaderPath = "")
	{
		const auto& [vertexSource, fragmentSource, geometrySource] = ParseShader(vertexShaderPath, fragmentShaderPath, geometryShaderPath);
		m_rendererID = CreateShader(vertexSource, fragmentSource, geometrySource);

#ifdef _DEBUG
		std::cout << "successfully create and compile shader: \n" << vertexShaderPath <<
			"\n" << fragmentShaderPath << "\n" << geometryShaderPath << "\n";
#endif 
	}

	~Shader()
	{
		glDeleteProgram(m_rendererID);
	}

	void Bind() const
	{
		glUseProgram(m_rendererID);
	}

	void Unbind() const
	{
		glUseProgram(0);
	}

	unsigned int GetID() const
	{
		return m_rendererID;
	}

	void SetVec3(const std::string& _name, const glm::vec3& value)
	{
		GLint location = glGetUniformLocation(m_rendererID, _name.c_str());

#ifdef _DEBUG
		if (location == -1 && warnedUniforms.find(_name) == warnedUniforms.end()) {
			std::cerr << "Warning: Uniform '" << _name << "' not found or shader program not linked.\n";
			warnedUniforms.insert(_name);
		}
#endif
		glUniform3fv(location, 1, &value[0]);
	}

	void SetVec3(const std::string& _name, float _x, float _y, float _z)
	{
		glm::vec3 value(_x, _y, _z);
		GLint location = glGetUniformLocation(m_rendererID, _name.c_str());

#ifdef _DEBUG
		if (location == -1 && warnedUniforms.find(_name) == warnedUniforms.end()) {
			std::cerr << "Warning: Uniform '" << _name << "' not found or shader program not linked.\n";
			warnedUniforms.insert(_name);
		}
#endif
		glUniform3fv(location, 1, &value[0]);
	}

	void SetVec2(const std::string& _name, const glm::vec2& value)
	{
		GLint location = glGetUniformLocation(m_rendererID, _name.c_str());

#ifdef _DEBUG
		if (location == -1 && warnedUniforms.find(_name) == warnedUniforms.end()) {
			std::cerr << "Warning: Uniform '" << _name << "' not found or shader program not linked.\n";
			warnedUniforms.insert(_name);
		}
#endif
		glUniform2fv(location, 1, &value[0]);
	}

	void SetMat4(const std::string& _name, const glm::mat4& _mat)
	{
		GLint location = glGetUniformLocation(m_rendererID, _name.c_str());

#ifdef _DEBUG
		if (location == -1 && warnedUniforms.find(_name) == warnedUniforms.end()) {
			std::cerr << "Warning: Uniform '" << _name << "' not found or shader program not linked.\n";
			warnedUniforms.insert(_name);
		}
#endif
		glUniformMatrix4fv(location, 1, GL_FALSE, &_mat[0][0]);
	}

	void SetMat3(const std::string& _name, const glm::mat3& _mat)
	{
		GLint location = glGetUniformLocation(m_rendererID, _name.c_str());

#ifdef _DEBUG
		if (location == -1 && warnedUniforms.find(_name) == warnedUniforms.end()) {
			std::cerr << "Warning: Uniform '" << _name << "' not found or shader program not linked.\n";
			warnedUniforms.insert(_name);
		}
#endif

		glUniformMatrix3fv(location, 1, GL_FALSE, &_mat[0][0]);
	}

	void SetFloat(const std::string& _name, float _value)
	{
		GLint location = glGetUniformLocation(m_rendererID, _name.c_str());

#ifdef _DEBUG
		if (location == -1 && warnedUniforms.find(_name) == warnedUniforms.end()) {
			std::cerr << "Warning: Uniform '" << _name << "' not found or shader program not linked.\n";
			warnedUniforms.insert(_name);
		}
#endif
		glUniform1f(location, _value);
	}

	
    //@brief Sets an integer or boolean uniform variable in a shader program.
    //
    // This function is versatile and serves two main purposes:
    // 1. Setting integer uniform variables in the shader.
    // 2. Specifying which texture unit a shader's texture sampler should use.
    //
    // @param _name The name of the uniform variable in the shader.
    // @param _value The integer or boolean value to set the uniform variable to.
    //
	void SetInt(const std::string& _name, int _value)
	{
		GLint location = glGetUniformLocation(m_rendererID, _name.c_str());

#ifdef _DEBUG
		if (location == -1 && warnedUniforms.find(_name) == warnedUniforms.end()) {
			std::cerr << "Warning: Uniform '" << _name << "' not found or shader program not linked.\n";
			warnedUniforms.insert(_name);
		}
#endif
		glUniform1i(location, _value);
	}

	void SetUniformBlock(const std::string& _name, const int bindingPoint) const
	{
		unsigned int blockIndex = glGetUniformBlockIndex(m_rendererID, _name.c_str());

#ifdef _DEBUG
		if (blockIndex == GL_INVALID_INDEX) 
			std::cerr << "Warning: Uniform block " << _name << " not found in shader." << std::endl;
#endif 
		glUniformBlockBinding(m_rendererID, blockIndex, bindingPoint);
	}

private:
	unsigned int CompileShader(unsigned int type, const std::string& source)
	{
		unsigned int id = glCreateShader(type);
		const char* src = source.c_str();
		glShaderSource(id, 1, &src, nullptr);
		glCompileShader(id);

#ifdef _DEBUG
		int result;
		glGetShaderiv(id, GL_COMPILE_STATUS, &result);

		if (result == GL_FALSE) {
			int length;
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
			std::vector<char> message(length);
			glGetShaderInfoLog(id, length, &length, message.data());
			std::string errorMessage = "Failed to compile ";

			if (type == GL_VERTEX_SHADER) errorMessage += "vertex";
			else if (type == GL_FRAGMENT_SHADER) errorMessage += "fragment";
			else if (type == GL_GEOMETRY_SHADER) errorMessage += "geometry";
			else errorMessage += "unknown";
			
			errorMessage += " shader: ";
			errorMessage += message.data();
			glDeleteShader(id);

			std::cout << errorMessage << "\n";
		}
#endif 
		return id;
	}

	std::tuple<std::string, std::string, std::string> ParseShader(const std::string& vertexShaderPath,
		const std::string& fragmentShaderPath, const std::string& geometryShaderPath)
	{
		std::ifstream vShaderFile(vertexShaderPath);
		std::ifstream fShaderFile(fragmentShaderPath);

		std::stringstream vShaderStream, fShaderStream, gShaderStream;

#ifdef  _DEBUG
		if (!vShaderFile.is_open())  
			std::cerr << "failed to open vertex shader file: " << vertexShaderPath; 
		
		if (!fShaderFile.is_open())
			std::cerr << "failed to open fragment shader file: " << fragmentShaderPath;
#endif 

		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();

		if (!geometryShaderPath.empty()) {
			std::ifstream gShaderFile(geometryShaderPath);
#ifdef _DEBUG
			if (!gShaderFile.is_open())
				std::cerr << "failed to open geometry shader file: " << geometryShaderPath;
#endif
			gShaderStream << gShaderFile.rdbuf();
			gShaderFile.close();
		}

		vShaderFile.close();
		fShaderFile.close();

		return std::make_tuple(vShaderStream.str(), fShaderStream.str(), gShaderStream.str());
	}

	unsigned int CreateShader(const std::string& vertexShader,
		const std::string& fragmentShader, const std::string& geometryShader)
	{
		unsigned int program = glCreateProgram();
		unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
		unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

		unsigned int gs = 0;
		if (!geometryShader.empty()) {
			gs = CompileShader(GL_GEOMETRY_SHADER, geometryShader);
			glAttachShader(program, gs);
		}
		glAttachShader(program, vs);
		glAttachShader(program, fs);
		glLinkProgram(program);
		glValidateProgram(program);

		glDeleteShader(vs);
		glDeleteShader(fs);
		if (gs)
			glDeleteShader(gs);

		return program;
	}

private:
	unsigned int m_rendererID; // Unique identifier for the OpenGL shader program
	std::unordered_set<std::string> warnedUniforms; // Set to keep track of uniform variables that have already triggered a warning
};

#endif // !SHADER_H