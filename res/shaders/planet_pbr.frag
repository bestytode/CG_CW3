#version 450 core
out vec4 FragColor;

in vec3 WorldPos;
in vec2 TexCoords;
in vec3 Normal;

uniform vec3 viewPos; // Camera (eye) position

// Material parameters
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

// Lighting infos
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform vec3 directionalLightDirection;
uniform vec3 directionalLightColor;

// Scaling factors
uniform float roughnessScale;
uniform float metallicScale;
uniform float directionalLightScale;
uniform vec3 albedoScale;

uniform float ka;
const float PI = 3.1415926535897932384626433832795;
const vec3 F0Base = vec3(0.04);

// Calculate the corresponding normal in world space
vec3 getNormalFromMap() {
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;
    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float NdotH = max(dot(N, H), 0.0);
    float denom = NdotH * (a - 1.0) + 1.0;
    return a / (PI * denom * denom);
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = geometrySchlickGGX(NdotV, roughness);
    float ggx2 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 albedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2)) * albedoScale;
    float metallic = texture(metallicMap, TexCoords).r * metallicScale;
    float roughness = texture(roughnessMap, TexCoords).r * roughnessScale;
    float ao = texture(aoMap, TexCoords).r;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(viewPos - WorldPos);

    vec3 F0 = mix(F0Base, albedo, metallic);

    // Positional Light calculation
    vec3 L = normalize(lightPosition - WorldPos);
    vec3 H = normalize(V + L);
    float distance = length(lightPosition - WorldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 incomingRadiance = lightColor * attenuation;
    float NdotL = max(dot(N, L), 0.0);

    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 specular = NDF * G * F / (4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001);

    vec3 Ks = F;
    vec3 Kd = (1.0 - Ks) * (1.0 - metallic);

    vec3 BRDF = Kd * albedo / PI + specular;
    vec3 Lo = BRDF * incomingRadiance * NdotL;

    // Directional light calculation
    vec3 L_dir = normalize(-directionalLightDirection);
    vec3 H_dir = normalize(V + L_dir);
    float NdotL_dir = max(dot(N, L_dir), 0.0);
    vec3 incomingRadiance_dir = directionalLightColor * directionalLightScale * NdotL_dir;

    float NDF_dir = distributionGGX(N, H_dir, roughness);
    float G_dir = geometrySmith(N, V, L_dir, roughness);
    vec3 F_dir = fresnelSchlick(max(dot(H_dir, V), 0.0), F0);

    vec3 specular_dir = NDF_dir * G_dir * F_dir / (4.0 * max(dot(N, V), 0.0) * NdotL_dir + 0.0001);
    vec3 BRDF_dir = (1.0 - F_dir) * albedo / PI + specular_dir;

    Lo += BRDF_dir * incomingRadiance_dir;

    vec3 ambient = vec3(ka) * albedo * ao;
    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}