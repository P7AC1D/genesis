set(PLAYGROUND_SOURCES
    src/main.cpp
    src/PlaygroundApp.cpp
    src/Sandbox.cpp
)

add_executable(GenesisPlayground ${PLAYGROUND_SOURCES})

target_include_directories(GenesisPlayground
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(GenesisPlayground
    PRIVATE
        GenesisEngine
)

# Copy assets to build directory
add_custom_command(TARGET GenesisPlayground POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:GenesisPlayground>/assets
)
