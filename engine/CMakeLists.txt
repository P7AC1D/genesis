set(ENGINE_SOURCES
    src/core/Application.cpp
    src/core/Window.cpp
    src/core/Log.cpp
    src/core/Layer.cpp
    src/core/LayerStack.cpp
    src/core/Input.cpp
    
    src/renderer/Renderer.cpp
    src/renderer/VulkanContext.cpp
    src/renderer/VulkanDevice.cpp
    src/renderer/VulkanSwapchain.cpp
    src/renderer/VulkanPipeline.cpp
    src/renderer/VulkanBuffer.cpp
    src/renderer/Mesh.cpp
    src/renderer/Camera.cpp
    src/renderer/Light.cpp
    
    src/ecs/Scene.cpp
    src/ecs/Entity.cpp
    src/ecs/Components.cpp
    
    src/procedural/Noise.cpp
    src/procedural/TerrainGenerator.cpp
    
    src/world/Chunk.cpp
    src/world/ChunkManager.cpp
    
    src/platform/PlatformDetection.cpp
)

# Platform-specific sources
if(GENESIS_PLATFORM_WINDOWS)
    list(APPEND ENGINE_SOURCES
        src/platform/windows/WindowsWindow.cpp
        src/platform/windows/WindowsInput.cpp
    )
elseif(GENESIS_PLATFORM_MACOS)
    list(APPEND ENGINE_SOURCES
        src/platform/macos/MacOSWindow.cpp
        src/platform/macos/MacOSInput.cpp
    )
elseif(GENESIS_PLATFORM_LINUX)
    list(APPEND ENGINE_SOURCES
        src/platform/linux/LinuxWindow.cpp
        src/platform/linux/LinuxInput.cpp
    )
endif()

add_library(GenesisEngine STATIC ${ENGINE_SOURCES})

target_include_directories(GenesisEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(GenesisEngine
    PUBLIC
        Vulkan::Vulkan
        glfw
        glm
)

target_precompile_headers(GenesisEngine PRIVATE
    <memory>
    <string>
    <vector>
    <unordered_map>
    <functional>
    <algorithm>
)
