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
├── playground/             # Feature testing sandbox
│   ├── src/               # Playground application
│   └── assets/            # Test assets and shaders
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
- **Push Constants**: Per-object transforms for efficient rendering

### Lighting System
- **Directional Light (Sun)**: With time-of-day simulation
- **Point Lights**: Up to 4 dynamic point lights with attenuation
- **Blinn-Phong Shading**: Diffuse and specular lighting
- **Time-of-Day**: Automatic sun position, color, and ambient transitions
- **Atmospheric Fog**: Distance-based fog with density control

### Procedural Generation
- **Simplex Noise**: 2D/3D noise with FBM and ridge noise variants
- **Terrain Generator**: Heightmap-based terrain with configurable parameters
- **Height-Based Coloring**: Water, sand, grass, rock, snow biome colors
- **Flat Shading**: Per-face normals for low-poly aesthetic

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

### Playground
- **WASD Camera Controller**: Free-fly camera with mouse look
- **Chunk World**: 7x7 chunk grid (224x224 world units)
- **Dynamic Lighting**: Time-of-day controls
- **Feature Testing**: Sandbox for developing new features

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

# Run playground
./build/playground/GenesisPlayground
```

### macOS with MoltenVK (Homebrew)

```bash
# Set environment variables before running
export VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json
export DYLD_LIBRARY_PATH=/opt/homebrew/lib

./build/playground/GenesisPlayground
```

### Build Options

```cmake
GENESIS_BUILD_EDITOR=ON      # Build the editor (default: ON)
GENESIS_BUILD_PLAYGROUND=ON  # Build the playground (default: ON)
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

### Creating Low-Poly Meshes

```cpp
auto& device = Application::Get().GetRenderer().GetDevice();

auto cube = Mesh::CreateCube(device, {1.0f, 0.0f, 0.0f});  // Red cube
auto tree = Mesh::CreateLowPolyTree(device);
auto rock = Mesh::CreateLowPolyRock(device);
```

### Procedural Terrain

```cpp
TerrainSettings settings;
settings.width = 64;
settings.depth = 64;
settings.heightScale = 8.0f;
settings.noiseScale = 0.04f;
settings.octaves = 5;
settings.flatShading = true;
settings.useHeightColors = true;

TerrainGenerator generator(settings);
auto terrainMesh = generator.Generate();
```

### Chunk-Based World

```cpp
WorldSettings worldSettings;
worldSettings.chunkSize = 32;
worldSettings.viewDistance = 3;  // 7x7 chunks
worldSettings.seed = 42;

ChunkManager chunkManager;
chunkManager.Initialize(device, worldSettings);

// In update loop:
chunkManager.Update(cameraPosition);

// In render loop:
chunkManager.Render(renderer);
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

### Renderer Pipeline

```
VulkanContext → VulkanDevice → VulkanSwapchain → VulkanPipeline
                                                        ↓
                                                  Command Buffer
                                                        ↓
                                    BeginScene(camera) → Draw(mesh, transform) → EndScene()
```

### Chunk System

```
ChunkManager::Update(cameraPosition)
    ↓
Determine visible chunks → Load new chunks → Unload distant chunks
    ↓
Chunk::Generate() → Noise sampling → Mesh generation → GPU upload
```

## Roadmap

### Completed
- [x] Vulkan rendering pipeline with synchronization
- [x] FPS camera with WASD controls
- [x] Procedural terrain generation with simplex noise
- [x] Chunk-based infinite world system
- [x] Lighting system with time-of-day
- [x] Point lights and fog

### Next Steps
- [x] **Water Rendering**: Flat water plane with transparency/reflection
- [ ] **Biome System**: Different terrain parameters based on world position
- [ ] **ImGui Integration**: Debug overlay with FPS, stats, and controls
- [ ] **Object Variation**: Random scale/rotation for trees and rocks
- [ ] **Shadow Mapping**: Basic directional light shadows

### Future
- [ ] Scene serialization/loading
- [ ] Physics integration
- [ ] Audio system
- [ ] More procedural objects (buildings, vegetation)
- [ ] Chunk LOD system
- [ ] Multithreaded chunk generation

## License

MIT License - See LICENSE file for details.
