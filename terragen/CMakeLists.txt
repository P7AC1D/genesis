set(TERRAGEN_SOURCES
    src/main.cpp
    src/TerragenApp.cpp
    src/Sandbox.cpp
    src/TerrainSettingsPanel.cpp
)

add_executable(Terragen ${TERRAGEN_SOURCES})

target_include_directories(Terragen
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Terragen
    PRIVATE
        GenesisEngine
)

# Copy assets to build directory
add_custom_command(TARGET Terragen POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:Terragen>/assets
)

# Convenience target: run terragen from the output directory so relative
# asset paths resolve correctly.
if(APPLE)
    # Try to detect Homebrew prefix; fall back to common locations.
    set(GENESIS_HOMEBREW_PREFIX "")
    execute_process(
        COMMAND brew --prefix
        OUTPUT_VARIABLE GENESIS_HOMEBREW_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT GENESIS_HOMEBREW_PREFIX)
        if(EXISTS "/opt/homebrew")
            set(GENESIS_HOMEBREW_PREFIX "/opt/homebrew")
        elseif(EXISTS "/usr/local")
            set(GENESIS_HOMEBREW_PREFIX "/usr/local")
        endif()
    endif()
    
    if(GENESIS_HOMEBREW_PREFIX)
        message(STATUS "Terragen: Found Homebrew at ${GENESIS_HOMEBREW_PREFIX}")
    else()
        message(WARNING "Terragen: Could not detect Homebrew prefix")
    endif()

    set(GENESIS_VK_ICD_JSON "${GENESIS_HOMEBREW_PREFIX}/etc/vulkan/icd.d/MoltenVK_icd.json")
    set(GENESIS_VK_LAYER_PATH "${GENESIS_HOMEBREW_PREFIX}/share/vulkan/explicit_layer.d")
    set(GENESIS_DYLD_LIBRARY_PATH "${GENESIS_HOMEBREW_PREFIX}/lib")

    if(GENESIS_HOMEBREW_PREFIX AND EXISTS "${GENESIS_VK_ICD_JSON}")
        add_custom_target(run_terragen
            COMMAND ${CMAKE_COMMAND} -E env
                VK_ICD_FILENAMES=${GENESIS_VK_ICD_JSON}
                VK_LAYER_PATH=${GENESIS_VK_LAYER_PATH}
                DYLD_LIBRARY_PATH=${GENESIS_DYLD_LIBRARY_PATH}
                ${CMAKE_COMMAND} -E chdir $<TARGET_FILE_DIR:Terragen> $<TARGET_FILE:Terragen>
            DEPENDS Terragen
        )
    else()
        add_custom_target(run_terragen
            COMMAND ${CMAKE_COMMAND} -E chdir $<TARGET_FILE_DIR:Terragen> $<TARGET_FILE:Terragen>
            DEPENDS Terragen
        )
    endif()
else()
    add_custom_target(run_terragen
        COMMAND ${CMAKE_COMMAND} -E chdir $<TARGET_FILE_DIR:Terragen> $<TARGET_FILE:Terragen>
        DEPENDS Terragen
    )
endif()

set_property(TARGET run_terragen PROPERTY USES_TERMINAL TRUE)
