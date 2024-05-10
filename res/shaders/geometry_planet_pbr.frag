#version 450 core
layout(location = 0) out vec4 FragColor;

uniform vec3 normal_color;

void main()
{
    FragColor = vec4(normal_color, 1.0f);
}