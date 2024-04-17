#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform bool reverse_normals;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
    FragPos = vec3(model * vec4(aPos, 1.0));

    // Adjusts the normals to render the inside surface of the cube, 
    // allowing proper lighting calculations as if you're looking from the inside of the cube, 
    // rather than the default outside view.
    if(reverse_normals) 
        Normal = transpose(inverse(mat3(model))) * (-1.0 * aNormal);
    else
        Normal = transpose(inverse(mat3(model))) * aNormal;

    TexCoords = aTexCoords;
}