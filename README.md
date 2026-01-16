# Genesis Engine

A cross-platform 3D low-poly game engine built with Vulkan, designed for creating procedurally generated worlds.

## Project Structure

```
genesis/
├── engine/                 # Core engine library
│   ├── include/genesis/    # Public headers
│   │   ├── core/          # Application, Window, Input, Layer system
│   │   ├── renderer/      # Vulkan renderer, Camera, Mesh, Lighting
│   │   ├── ecs/           # Entity Component System
│   │   ├── procedural/    # Noise, Terrain generation
│   │   ├── world/         # Chunk system, World management
│   │   ├── math/          # Math utilities
│   │   └── platform/      # Platform abstractions
│   ├── src/               # Implementation files
│   └── vendor/            # Third-party dependencies
│
├── editor/                 # Genesis Editor (Unity/Unreal-style)
│   ├── include/editor/    # Editor headers
│   ├── src/               # Editor implementation
│   │   └── Panels/        # UI panels (Hierarchy, Properties, etc.)
│   └── assets/            # Editor-specific assets
│
├── terragen/               # Terrain generation sandbox
│   ├── src/               # Terragen application
│   └── assets/            # Shaders and test assets
│
└── cmake/                  # CMake modules
```

## Features

### Core Engine
- **Vulkan Rendering Backend**: Modern graphics API with synchronization
- **Cross-Platform**: Windows, macOS (MoltenVK), Linux support via GLFW
- **Entity Component System**: Flexible game object architecture
- **Layer System**: Modular application layers
- **Input System**: Keyboard and mouse input handling

### Renderer
- **Full Render Loop**: Frame synchronization, command buffers, descriptor sets
- **Low-Poly Mesh Generation**: Built-in primitives (cube, plane, icosphere)
- **Procedural Meshes**: Trees, rocks with flat shading for low-poly style
- **Camera System**: FPS-style camera with perspective projection
- **Water Rendering**: Transparent water with wave animation and fresnel effect

### Lighting System
- **Directional Light (Sun)**: With time-of-day simulation
- **Point Lights**: Up to 4 dynamic point lights with attenuation
- **Blinn-Phong Shading**: Diffuse and specular lighting
- **Time-of-Day**: Automatic sun position, color, and ambient transitions
- **Atmospheric Fog**: Distance-based fog with density control

### Procedural Generation
- **Simplex Noise**: 2D/3D noise with FBM and ridge noise variants
- **Domain Warping**: Multi-level coordinate warping for organic terrain
- **Terrain Generator**: Heightmap-based terrain with configurable parameters
- **Height-Based Coloring**: Water, sand, grass, rock, snow colors

### Chunk-Based World System
- **Infinite Terrain**: Dynamic chunk loading/unloading
- **Seamless Chunks**: World-space noise sampling for continuous terrain
- **View Distance**: Configurable chunk render distance
- **Object Placement**: Procedural trees and rocks per chunk

### Editor (In Progress)
- **Scene Hierarchy Panel**: Entity management
- **Properties Panel**: Component editing
- **Viewport Panel**: 3D scene view
- **Asset Browser**: Project file navigation

### Terragen
- **WASD Camera Controller**: Free-fly camera with mouse look
- **Chunk World**: 7x7 chunk grid with procedural terrain
- **Dynamic Lighting**: Time-of-day controls
- **Water Rendering**: Sea level with animated waves

## Controls

| Key | Action |
|-----|--------|
| W/S | Move forward/backward |
| A/D | Strafe left/right |
| Space | Move up |
| Left Shift | Move down |
| Right Mouse + Move | Look around |
| T | Advance time of day |
| Shift + T | Reverse time of day |

## Dependencies

- **Vulkan SDK**: Graphics API (required)
- **GLFW**: Cross-platform windowing (auto-fetched)
- **GLM**: Mathematics library (auto-fetched)

## Building

### Prerequisites

1. Install Vulkan SDK from https://vulkan.lunarg.com/
2. CMake 3.20 or higher
3. C++20 compatible compiler

### Build Commands

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build all targets
cmake --build build

# Run terragen
./build/terragen/Terragen
```

### macOS with MoltenVK (Homebrew)

```bash
# Set environment variables before running
export VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json
export DYLD_LIBRARY_PATH=/opt/homebrew/lib

./build/terragen/Terragen

# Or use the convenience target
cmake --build build --target run_terragen
```

### Build Options

```cmake
GENESIS_BUILD_EDITOR=ON     # Build the editor (default: ON)
GENESIS_BUILD_TERRAGEN=ON   # Build the terragen app (default: ON)
```

## Usage

### Creating a New Application

```cpp
#include <genesis/Genesis.h>

class MyApp : public Genesis::Application {
public:
    MyApp(const Genesis::ApplicationConfig& config)
        : Application(config) {}

protected:
    void OnInit() override {
        PushLayer(new MyGameLayer());
    }
};

Genesis::Application* Genesis::CreateApplication(int argc, char** argv) {
    Genesis::ApplicationConfig config;
    config.Name = "My Game";
    config.Window.Title = "My Game";
    config.Window.Width = 1280;
    config.Window.Height = 720;
    return new MyApp(config);
}
```

### Terrain Generation

```cpp
WorldSettings worldSettings;
worldSettings.chunkSize = 32;
worldSettings.cellSize = 1.0f;
worldSettings.viewDistance = 3;

// Configure terrain settings
worldSettings.terrainSettings.heightScale = 10.0f;
worldSettings.terrainSettings.noiseScale = 0.03f;
worldSettings.terrainSettings.octaves = 5;
worldSettings.terrainSettings.persistence = 0.5f;
worldSettings.terrainSettings.useWarp = true;  // Domain warping

ChunkManager chunkManager;
chunkManager.Initialize(device, worldSettings);
```

### Lighting

```cpp
auto& lightManager = Application::Get().GetRenderer().GetLightManager();

// Time of day (0-24 hours)
lightManager.SetTimeOfDay(14.0f);  // 2 PM

// Add point light
Light campfire;
campfire.Position = glm::vec3(5.0f, 2.0f, 5.0f);
campfire.Color = glm::vec3(1.0f, 0.6f, 0.2f);
campfire.Intensity = 2.0f;
lightManager.AddPointLight(campfire);

// Enable fog
lightManager.SetFogDensity(0.01f);
```

## Architecture

### Application Flow

```
main() → CreateApplication() → Application::Run()
    ↓
Application Loop:
    1. PollEvents()
    2. Layer::OnUpdate(deltaTime)
    3. Renderer::BeginFrame()
    4. Layer::OnRender()
    5. Renderer::EndFrame()
```

### Terrain Generation Pipeline

```
1. FBM Noise             → Base terrain shape
2. Domain Warping        → Organic, non-uniform features
3. Height Coloring       → Water, sand, grass, rock, snow
4. Mesh Generation       → Flat-shaded triangles
```

### Chunk System

```
ChunkManager::Update(cameraPosition)
    ↓
Determine visible chunks → Load new chunks → Unload distant chunks
    ↓
Chunk::Generate() → Heightmap → Mesh generation → Object placement
```

## Next Steps

Planned improvements for the terrain system:
1. **Ridge Noise** - Sharp mountain crests and spines
2. **Tectonic Uplift Mask** - Mountain ranges in bands
3. **Hydraulic Erosion** - Valleys and drainage patterns
4. **Biome System** - Climate-based terrain variation
5. **LOD System** - Level of detail for distant terrain

## License

MIT License - See LICENSE file for details.
