#version 450 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;

uniform float exposure;
uniform float sceneScale;

void main()
{
	const float gamma = 2.2f;
	vec3 hdrColor = texture(scene, TexCoords).rgb;
	vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;

	hdrColor += 1.0f * bloomColor; // additive blending
    
    // tone mapping
    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);

    // also gamma correct while we're at it       
    result = pow(result, vec3(1.0 / gamma));
    result = result * sceneScale;

    FragColor = vec4(result, 1.0);
}