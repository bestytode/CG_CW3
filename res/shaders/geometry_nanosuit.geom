#version 450 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec2 texCoords;
} gs_in[];

out vec2 TexCoords;

uniform float time;          // Current time
uniform float startTime;     // Start time of the explosion
uniform float duration;      // Duration of the explosion

vec4 explode(vec4 position, vec3 normal)
{
    // Calculate the elapsed time since the start of the explosion
    float elapsed = time - startTime;
    
    // Determine the normalized progression of the explosion, capping it at 1.0
    float progression = clamp(elapsed / duration, 0.0, 1.0);
    
    // Define the maximum magnitude of the explosion
    float magnitude = 2.0;  // Adjust as needed
    
    // Calculate the explosion vector
    vec3 explosionVec = normal * magnitude * progression;  // Linear progression until peak
    return position + vec4(explosionVec, 0.0);
}

vec3 GetNormal()
{
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross(a, b));
}

void main() {
    vec3 normal = GetNormal();

    for (int i = 0; i < 3; i++) {
        gl_Position = explode(gl_in[i].gl_Position, normal);
        TexCoords = gs_in[i].texCoords;
        EmitVertex();
    }

    EndPrimitive();
}
