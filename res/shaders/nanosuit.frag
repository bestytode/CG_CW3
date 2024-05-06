#version 450 core

in vec2 TexCoords;
in vec3 Normal;
in vec3 WorldPos;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform vec3 viewPos; // Camera position for specular calculation

// Light parameters
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform vec3 directionalLightDirection;
uniform vec3 directionalLightColor;
uniform float directionalLightScale;

uniform float ka;  // Ambient coefficient
uniform float kd;  // Diffuse coefficient
uniform float ks;  // Specular coefficient
uniform float shininess;  // Specular shininess factor

out vec4 FragColor;

void main()
{
    vec3 normal = normalize(Normal);

    // Ambient component
    vec3 ambient = ka * texture(texture_diffuse1, TexCoords).rgb;

    // Positional light calculations
    vec3 lightDir = normalize(lightPosition - WorldPos);
    float distance = length(lightPosition - WorldPos);
    float attenuation = 1.0f / (distance * distance + 0.32f * distance + 1.0f);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = attenuation * kd * diff * lightColor * texture(texture_diffuse1, TexCoords).rgb;
    
    vec3 viewDir = normalize(viewPos - WorldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float specStrength = texture(texture_specular1, TexCoords).r;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = ks * specStrength * spec * lightColor; 

    // Directional light calculations
    lightDir = normalize(-directionalLightDirection);
    diff = max(dot(normal, lightDir), 0.0);
    diffuse += directionalLightScale * kd * diff * directionalLightColor * texture(texture_diffuse1, TexCoords).rgb;
    
    spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    specular += directionalLightScale * ks * specStrength * spec * directionalLightColor;

    // Combine lighting results
    vec3 result = ambient + diffuse + specular;

    // Apply tone mapping
    result = result / (result + vec3(1.0));
    result = pow(result, vec3(1.0/2.2));

    FragColor = vec4(result, 1.0);
}