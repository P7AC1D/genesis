set(EDITOR_SOURCES
    src/main.cpp
    src/EditorApp.cpp
    src/EditorLayer.cpp
    src/Panels/SceneHierarchyPanel.cpp
    src/Panels/PropertiesPanel.cpp
    src/Panels/ViewportPanel.cpp
    src/Panels/AssetBrowserPanel.cpp
)

add_executable(GenesisEditor ${EDITOR_SOURCES})

target_include_directories(GenesisEditor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/editor
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(GenesisEditor
    PRIVATE
        GenesisEngine
)

# Copy assets to build directory
add_custom_command(TARGET GenesisEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:GenesisEditor>/assets
)
