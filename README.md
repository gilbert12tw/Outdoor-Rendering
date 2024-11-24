# Outdoor Environment Rendering

This project showcases various rendering techniques and optimizations for rendering outdoor environments. The goal is to achieve efficient and visually appealing rendering for scenes containing foliage, buildings, and other outdoor elements.

## Features

### Basic Features
- **Render Scene Correctly**: Ensures the scene is rendered without errors. 
- **Blinn-Phong Shading**: Implements Blinn-Phong shading for realistic lighting.
- **Deferred Shading**: Enables efficient lighting calculations for complex scenes. 
- **Normal Mapping**: Enhances surface detail using normal maps. 
- **Indirect Instancing for Foliage and Buildings**: All foliage and buildings are rendered using a single indirect draw call for optimal performance.
- **GPU-Driven View-Frustum Culling**: Implements view-frustum culling using bounding spheres for efficient rendering.

### Advanced Features
- **Hierarchical-Z Occlusion Culling**: Uses hierarchical-Z data for accurate and efficient occlusion culling.
- **Correct Occlusion Culling**: Implements terrain, magic stones, buildings, and airplanes as occluders to optimize visibility checks.
- **Depth Mipmap Display**: Displays depth mipmaps at various levels for debugging and optimization purposes.

## Requirements
- **Hardware**: NVIDIA RTX 2080 or equivalent GPU for optimal performance.
- **Software**: Visual studio with OpenGL
