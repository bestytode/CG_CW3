#version 330 core
out float FragColor; // only red channel in texture

in vec2 TexCoords;
uniform sampler2D ssaoInput; // raw occlusion coefficient

void main()
{
	vec2 texelSize = 1.0f / vec2(textureSize(ssaoInput, 0));
	float result = 0.0f;

	for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoInput, TexCoords + offset).r;
        }
    }
    FragColor = result / (4.0 * 4.0);
}