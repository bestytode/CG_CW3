#version 450 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    TexCoords = aPos;
    vec4 pos = projection * view * model * vec4(aPos, 1.0);

    // this will ensure z value (depth value)after projection always be 1.0f
    // we can also manually set gl_FragDepth to 1.0f in fragment shader
    gl_Position = pos.xyww; 
}  