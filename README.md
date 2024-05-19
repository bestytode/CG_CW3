# CG Project Coursework

## Models
- **Rocks**: See `res/models/rock`, parsing data by `src/model.h`
- **Planet**: See `src/geometry_renderer.h`
- **Nanosuit**: See `res/models/nanosuit`, parsing data by `src/model.h`

## Camera
- See `src/camera.h`

## Texture
- **PBR Texture**: See `src/pbr.h`, function `LoadPBRTexture`
- **Skybox**: See `src/skybox.h`, function `LoadCubemap`

## Lighting
A positional and a directional HDR light. See `src/config.h` and glsl code.

## Shadow
Since point shadow not suitable in space environment, use darker side representing shadow

## Curves
See `src/config.h`, function `UpdatePositionalLight`

## Transparency
See `res/shaders/geometry_nanosuit.geom`

## R&D
-- **Skybox**: some core technics summary: 1. 1st render in render loop. 2. Explicitly ensure z value after projection keep be 1.0f 3. remove translation from the view matrix.

-- **Instancing Rendering**: The core is sending a large array of matrices only once to reduce the number of draw calls,  improving performance ultimately. Implmentation see `src/instancing.h`

-- **Geometry Shader**: geometry shader used to process primitives, in this project I use this to explicitly render normal(they look like fur tbh) with yellow color. 

-- **PBR material**: See following.

### Microfacets Theory
The code is grounded on Microfacets Theory, taking into account the microscopic surface details when rendering materials.

### Rendering Function
The core of this repository lies in the rendering function, which computes both lighting and material appearance.

$$
\text{Final Color} = \frac{\text{Albedo}}{\pi} + \text{Specular}
$$


To understand the full scope of our rendering process, consider the extended rendering equation:

$$
L_o(p, \omega_o) = L_e(p, \omega_o) + \int_{\Omega} f(p, \omega_o, \omega_i) \times L_i(p, \omega_i) \times (\omega_i \cdot n) d\omega_i
$$

### Explanations for Rendering Equation Parameters

- **( $L_o(p, \omega_o)$ )**: The outgoing radiance at point $( p )$ in direction $( \omega_o )$.

- **( $L_e(p, \omega_o)$ )**: The emitted radiance from point $( p )$ in direction $( \omega_o )$.

- **( $f(p, \omega_o, \omega_i)$ )**: The BRDF at point $( p )$.

- **( $L_i(p, \omega_i)$ )**: The incoming radiance at point $( p )$ in direction $( \omega_i )$.

- **( $\omega_i \cdot n$ )**: The dot product between the incoming light direction and the surface normal, affecting how much light is received.

### BRDF Function
The Bidirectional Reflectance Distribution Function (BRDF) is implemented in detail to provide high-fidelity rendering.

- **Distribution (D) Function**

$$
D(h) = \frac{\alpha^2}{\pi \times ((\alpha^2 - 1) \times (1 + (\alpha^2 - 1) \times (h \cdot n)^2))}
$$

- **Fresnel (F) Function**

$$
F(\theta) = F_0 + (1 - F_0) \times (1 - \cos(\theta))^5
$$

- **Geometry (G) Function**

$$
G = G_1(N \cdot V) \times G_1(N \cdot L)
$$

The $( G_1 )$ term is calculated as follows:

$$
G_1(v) = \frac{2}{1 + \sqrt{1 + \alpha^2 \times (1 - (v \cdot n)^2) / (v \cdot n)^2}}
$$

### Explanations for BRDF Parameters

- **( $\alpha$ ) (roughness coefficient)**: This is the roughness parameter, affecting the spread of microfacets. A smaller value leads to a smoother surface, while a larger value results in a rougher surface.

- **( $F_0$ ) (Fresnel-Schlick Approximation)**: The value of reflectance at zero angle of incidence, which defines how reflective the material is.

- **( $N$ ) (Normal Vector)**: The surface normal vector at the point of interest.

- **( $V$ ) (View Vector)**: The vector pointing from the surface point to the camera.

- **( $L$ ) (Light Vector)**: The vector pointing from the surface point to the light source.

### Corresponding Items in project (All in res/shaders/planet_pbr.frag)
-- **D function**: distributionGGX() line 49.  
-- **F function**: fresnelSchlick() line 69.  
-- **G function**: geometrySmith() line 61.  
-- **roughness, albedo, metallic, and ao**: all from map, and Fo will be calculated as: vec3 F0 = mix(F0Base, albedo, metallic), where F0Base is 0.4f(by default).  
-- **Ks**: directly the result of function fresnelSchlick.  
-- **Kd**: based on ENERGY CONSERVATION, calculated as vec3 Kd = (1.0 - Ks) * (1.0 - metallic).  
-- **BRDF**: line 101, vec3 BRDF = Kd * albedo / PI + specular, where PI is the intergral of hemisphere to keep energy conservation.  
