#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D noiseTexture;

uniform float kernelSize;
uniform vec3 samples[16];
uniform float radius;

uniform mat4 projection;

// tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(1920.0f / 4.0f, 1080.0f / 4.0f); 

void main()
{
    // Retrieve Position and Normal from G-Buffer, noise vector from noiseTexture
    vec3 FragPos = texture(gPosition, TexCoords).xyz;
    vec3 Normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(noiseTexture, TexCoords * noiseScale).xyz);

    // Create TBN change-of-basis matrix: from tangent-space to view-space
    // The TBN matrix is used to transform the sample points into an orientation
    // that aligns with the fragment's normal. This is essential for correct SSAO.
    // The TBN matrix also incorporates randomness through randomVec to make
    // the SSAO look more natural and avoid visual artifacts.
    vec3 tangent = normalize(randomVec - Normal * dot(randomVec, Normal));
    vec3 bitangent = cross(Normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, Normal);

    // SSAO Kernel Loop
    float occlusion = 0.0f;
    for(int i = 0; i < kernelSize; i++) {
        // Transform the sample point from tangent space to view space.
        // The sample point is fetched from a pre-computed array of points that
        // are oriented along the z-axis in tangent space.
        vec3 sample = TBN * samples[i]; // tangent space -> view space
        sample = FragPos + sample * radius;

        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(sample, 1.0f);
        offset = projection * offset; // view space -> clip space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5f + 0.5; // transform to range 0.0 - 1.0

        // Retrieve the linearized depth value from the gPositionDepth texture.
        float sampleDepth = texture(gPosition, offset.xy).z; 

        // Introduce a rangeCheck to ensure that it only affect the occlusion factor when the measured depth value is within the radius
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(FragPos.z - sampleDepth));

        // Compare the actual depth of the scene at the sample point (sampleDepth)
        // with the depth of the sample point itself (sample.z).
        // If sampleDepth is greater, it means the sample point is not occluded.
        occlusion += (sampleDepth >= sample.z + 0.025f ? 1.0 : 0.0) * rangeCheck;   
    }

    // Normalization & inversion for later use
    // serves as a coefficient can use to modulate the lighting
    occlusion = 1.0 - (occlusion / kernelSize);
    FragColor = occlusion;
}