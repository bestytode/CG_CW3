#version 450 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D image;
uniform bool horizontal;

// pre-calculated Gaussian weights that determine how much each neighboring pixel (texel) contributes to
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
	vec2 tex_offset = 1.0f / textureSize(image, 0);
	vec3 result = texture(image, TexCoords).rgb * weight[0];

	if (horizontal) {
		for (int i = 1; i < 5; i++) {
			result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
			result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
		}
	}
	else {
		for (int j = 1; j < 5; j++) {
			result += texture(image, TexCoords + vec2(0.0f, tex_offset.y * j)).rgb * weight[j];
			result += texture(image, TexCoords - vec2(0.0f, tex_offset.y * j)).rgb * weight[j];
		}
	}
	FragColor = vec4(result, 1.0f);
}