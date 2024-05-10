#version 450 core
layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform float ka;

void main()
{
    vec3 color = 20.0f * ka * texture(texture_diffuse1, TexCoords).rgb;
    FragColor = vec4(color, 1.0f);
}