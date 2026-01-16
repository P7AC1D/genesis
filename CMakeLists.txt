cmake_minimum_required(VERSION 3.20)
project(Genesis VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(GENESIS_BUILD_EDITOR "Build the Genesis Editor" ON)
option(GENESIS_BUILD_PLAYGROUND "Build the Genesis Playground" ON)

# Platform detection
if(WIN32)
    set(GENESIS_PLATFORM_WINDOWS ON)
    add_compile_definitions(GENESIS_PLATFORM_WINDOWS)
elseif(APPLE)
    set(GENESIS_PLATFORM_MACOS ON)
    add_compile_definitions(GENESIS_PLATFORM_MACOS)
elseif(UNIX)
    set(GENESIS_PLATFORM_LINUX ON)
    add_compile_definitions(GENESIS_PLATFORM_LINUX)
endif()

# Find Vulkan
find_package(Vulkan REQUIRED)

# Include cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Third-party dependencies
add_subdirectory(engine/vendor)

# Core engine library
add_subdirectory(engine)

# Editor application
if(GENESIS_BUILD_EDITOR)
    add_subdirectory(editor)
endif()

# Playground application
if(GENESIS_BUILD_PLAYGROUND)
    add_subdirectory(playground)
endif()
