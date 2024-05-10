#version 450 core

layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform float time;          // Current time
uniform float startTime;     // When the explosion starts
uniform float duration;      // How long the explosion effect lasts

void main()
{
    vec3 color = 2.0 * texture(texture_diffuse1, TexCoords).rgb;
    
    // Calculate the elapsed time relative to the explosion start
    float elapsed = time - startTime;
    
    // Calculate alpha based on elapsed time within the duration
    float alpha = 1.0 - clamp(elapsed / duration, 0.0, 1.0); // Fades out as time progresses

    FragColor = vec4(color, alpha);
}