#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D diffuseTexture;
uniform samplerCube depthMap;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform float far_plane;
uniform bool shadows;

// array of offset direction for sampling
vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

float ShadowCalculation();
float ShadowCalculation_3dimensions();

void main()
{
	vec3 lightDir = normalize(lightPos - FragPos);
	vec3 normal = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 color = texture(diffuseTexture, TexCoords).rgb;
	vec3 lightColor = vec3(0.5f);

	// Ambient
	vec3 ambient = 0.3 * lightColor;

	// Diffuse
	float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;

	// Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor; 
	
	// Calculate shadow
    float shadow = shadows ? ShadowCalculation() : 0.0f;                      
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;   

	FragColor = vec4(lighting, 1.0f);
}

float ShadowCalculation()
{
    // We use a vector from light to fragment as texture coordinates
    vec3 fragToLight = FragPos - lightPos;
    float currentDepth = length(fragToLight);

    float bias = 0.05;
    float shadow = 0.0f;

    for(int i = 0; i < 20; i++) {
        float closetDepth = texture(depthMap, fragToLight + 0.05f * gridSamplingDisk[i]).r;
        closetDepth *= far_plane; // Undo mapping [0, 1]
        shadow += (currentDepth - bias > closetDepth) ? 1.0f : 0.0f;
    }
        
    return shadow / 20.0f;
}


float ShadowCalculation_3dimensions()
{
    vec3 frag_light = FragPos - lightPos;
    float current = length(frag_light);

    float shadow = 0.0f;
    float bias = 0.05;
    for(float i = -0.05; i <= 0.05; i += 0.05) {
        for(float j = -0.05; j <= 0.05; j += 0.05) {
            for(float k = -0.05; k <= 0.05; k += 0.05) {
                float closet = texture(depthMap, frag_light + vec3(i, j, k)).r;
                closet *= far_plane; // Undo mapping [0, 1]
                shadow += (current - bias > closet) ? 1.0f : 0.0f;
            }
        }
    }
    return shadow / 27;
}