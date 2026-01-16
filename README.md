# Genesis Engine

A cross-platform 3D low-poly game engine built with Vulkan, designed for creating procedurally generated worlds.

## Project Structure

```
genesis/
├── engine/                 # Core engine library
│   ├── include/genesis/    # Public headers
│   │   ├── core/          # Application, Window, Input, Layer system
│   │   ├── renderer/      # Vulkan renderer, Camera, Mesh
│   │   ├── ecs/           # Entity Component System
│   │   ├── math/          # Math utilities, procedural helpers
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
│   └── assets/            # Test assets
│
└── cmake/                  # CMake modules
```

## Features

### Core Engine
- **Vulkan Rendering Backend**: Modern graphics API for high performance
- **Cross-Platform**: Windows, macOS, Linux support via GLFW
- **Entity Component System**: Flexible game object architecture
- **Layer System**: Modular application layers
- **Input System**: Keyboard and mouse input handling

### Renderer
- **Low-Poly Mesh Generation**: Built-in primitives (cube, plane, icosphere)
- **Procedural Mesh Support**: Trees, rocks, terrain generation
- **Camera System**: Perspective and orthographic cameras
- **Depth Buffer**: Proper 3D rendering with depth testing

### Editor
- **Scene Hierarchy Panel**: Entity management
- **Properties Panel**: Component editing
- **Viewport Panel**: 3D scene view
- **Asset Browser**: Project file navigation

### Playground
- **WASD Camera Controller**: Free-fly camera for testing
- **Test Scene**: Pre-configured scene with test objects
- **Feature Testing**: Sandbox for developing new features

## Dependencies

- **Vulkan SDK**: Graphics API (required)
- **GLFW**: Cross-platform windowing
- **GLM**: Mathematics library
- **stb**: Image loading (header-only)

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

# Build specific target
cmake --build build --target GenesisPlayground

# Run playground
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
        // Initialize your game
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

### Creating Entities

```cpp
auto entity = scene->CreateEntity("Player");
// Add components (when ECS is fully implemented)
// entity.AddComponent<TransformComponent>();
// entity.AddComponent<MeshRendererComponent>(mesh);
```

### Creating Low-Poly Meshes

```cpp
auto& device = Application::Get().GetRenderer().GetDevice();

auto cube = Mesh::CreateCube(device, {1.0f, 0.0f, 0.0f});  // Red cube
auto tree = Mesh::CreateLowPolyTree(device);
auto rock = Mesh::CreateLowPolyRock(device);
```

## Roadmap

- [ ] Complete Vulkan rendering pipeline
- [ ] ImGui integration for editor UI
- [ ] Procedural terrain generation
- [ ] Scene serialization
- [ ] Physics integration
- [ ] Audio system
- [ ] Scripting support
- [ ] Asset pipeline

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
                                                 Render Commands
```

## License

MIT License - See LICENSE file for details.
