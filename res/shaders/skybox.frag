#version 450 core
layout(location = 0) out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    //gl_FragDepth = 1.0f;
    FragColor = texture(skybox, TexCoords);
}
