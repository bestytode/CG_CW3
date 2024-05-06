#version 450 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform float ka;

void main()
{
    FragColor = 20.0f * ka * texture(texture_diffuse1, TexCoords);
}