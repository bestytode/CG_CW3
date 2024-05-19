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
-- **Instancing Rendering**: The core is sending a large array of matrices only once to reduce the number of draw calls, 
improving performance ultimately. Implmentation see `src/instancing.h`
-- **Geometry Shader**: geometry shader used to process primitives, in this project I use this to explicitly render normal(they look like fur tbh) with yellow color.
-- **PBR material**:
