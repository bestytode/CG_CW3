#pragma once
#ifndef BLOOM_H
#define BLOOM_H

#include "shader.h"
#include "geometry_renderers.h"

void RenderBloomLightSource(Shader& bloomShader, yzh::Sphere& sphere)
{
	bloomShader.Bind();
	sphere.Render();
}

#endif // !BLOOM_H