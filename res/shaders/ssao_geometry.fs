#version 330 core
layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec3 gAlbedo;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

void main()
{
    // save fragPos in view space
    gPosition = FragPos;
    gNormal = normalize(Normal);
    gAlbedo = vec3(0.95f);
}