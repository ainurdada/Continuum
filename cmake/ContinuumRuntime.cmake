function(continuum_configure_runtime applicationTarget)
set(CONTINUUM_RUNTIME_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")

set_target_properties(
    ${applicationTarget}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CONTINUUM_RUNTIME_DIRECTORY}"
)

if(WIN32)
    set(SHADERCROSS_EXECUTABLE "${PROJECT_SOURCE_DIR}/third_party/SDL/SDL3_shadercross-3.0.0-windows-VC-x64/bin/shadercross.exe")
elseif(APPLE)
    set(SHADERCROSS_EXECUTABLE "${PROJECT_SOURCE_DIR}/third_party/SDL/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross")
else()
    message(FATAL_ERROR "Not supported platform")
endif()

if(NOT EXISTS "${SHADERCROSS_EXECUTABLE}")
    message(FATAL_ERROR "shadercross executable not found: ${SHADERCROSS_EXECUTABLE}")
endif()

set(VERTEX_SHADER_SOURCE "${PROJECT_SOURCE_DIR}/shaders/default.vert.hlsl")
set(FRAGMENT_SHADER_SOURCE "${PROJECT_SOURCE_DIR}/shaders/default.frag.hlsl")
set(FRAGMENT_SHADER_SOURCE_GRID "${PROJECT_SOURCE_DIR}/shaders/grid.frag.hlsl")
if(APPLE)
    set(VERTEX_SHADER_OUTPUT "${CONTINUUM_RUNTIME_DIRECTORY}/shaders/default.vert.msl")
    set(FRAGMENT_SHADER_OUTPUT "${CONTINUUM_RUNTIME_DIRECTORY}/shaders/default.frag.msl")
    set(FRAGMENT_SHADER_OUTPUT_GRID "${CONTINUUM_RUNTIME_DIRECTORY}/shaders/grid.frag.msl")
    set(SHADER_DESTINATION_FORMAT "MSL")
elseif(WIN32)
    set(VERTEX_SHADER_OUTPUT "${CONTINUUM_RUNTIME_DIRECTORY}/shaders/default.vert.dxil")
    set(FRAGMENT_SHADER_OUTPUT "${CONTINUUM_RUNTIME_DIRECTORY}/shaders/default.frag.dxil")
    set(FRAGMENT_SHADER_OUTPUT_GRID "${CONTINUUM_RUNTIME_DIRECTORY}/shaders/grid.frag.dxil")
    set(SHADER_DESTINATION_FORMAT "DXIL")
endif()

add_custom_command(
    OUTPUT "${VERTEX_SHADER_OUTPUT}"
    
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${CONTINUUM_RUNTIME_DIRECTORY}/shaders"

    COMMAND "${SHADERCROSS_EXECUTABLE}"
        "${VERTEX_SHADER_SOURCE}"
        -s HLSL
        -d "${SHADER_DESTINATION_FORMAT}"
        -t vertex
        -o "${VERTEX_SHADER_OUTPUT}"

    COMMENT "Compiling vertex shader"
    DEPENDS "${VERTEX_SHADER_SOURCE}"
    VERBATIM
)

add_custom_command(
    OUTPUT "${FRAGMENT_SHADER_OUTPUT}"
    
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${CONTINUUM_RUNTIME_DIRECTORY}/shaders"

    COMMAND "${SHADERCROSS_EXECUTABLE}"
        "${FRAGMENT_SHADER_SOURCE}"
        -s HLSL
        -d "${SHADER_DESTINATION_FORMAT}"
        -t fragment
        -o "${FRAGMENT_SHADER_OUTPUT}"

    COMMENT "Compiling fragment shader"
    DEPENDS "${FRAGMENT_SHADER_SOURCE}"
    VERBATIM
)

add_custom_command(
    OUTPUT "${FRAGMENT_SHADER_OUTPUT_GRID}"

    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${CONTINUUM_RUNTIME_DIRECTORY}/shaders"

    COMMAND "${SHADERCROSS_EXECUTABLE}"
        "${FRAGMENT_SHADER_SOURCE_GRID}"
        -s HLSL
        -d "${SHADER_DESTINATION_FORMAT}"
        -t fragment
        -o "${FRAGMENT_SHADER_OUTPUT_GRID}"

    COMMENT "Compiling fragment shader"
    DEPENDS "${FRAGMENT_SHADER_SOURCE_GRID}"
    VERBATIM
)

add_custom_target(
    ${applicationTarget}Shaders
    DEPENDS "${VERTEX_SHADER_OUTPUT}" "${FRAGMENT_SHADER_OUTPUT}" "${FRAGMENT_SHADER_OUTPUT_GRID}"
)

add_dependencies(${applicationTarget} ${applicationTarget}Shaders)

if(WIN32)
    add_custom_command(
        TARGET ${applicationTarget}
        POST_BUILD

        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3-shared>"
            "$<TARGET_FILE_DIR:${applicationTarget}>"

        COMMENT "Copying SDL3.dll next to ${applicationTarget}.exe"
        VERBATIM
    )
endif()
endfunction()

function(
    continuum_copy_runtime_file
    applicationTarget
    copyTarget
    sourceFile
    outputDirectory
)
    if(NOT TARGET "${applicationTarget}")
        message(FATAL_ERROR "Unknown application target: ${applicationTarget}")
    endif()

    if(NOT EXISTS "${sourceFile}")
        message(FATAL_ERROR "Runtime source file not found: ${sourceFile}")
    endif()

    get_target_property(
        runtimeDirectory
        "${applicationTarget}"
        RUNTIME_OUTPUT_DIRECTORY
    )

    if(NOT runtimeDirectory)
        message(FATAL_ERROR "Runtime directory is not configured for: ${applicationTarget}")
    endif()

    get_filename_component(sourceFileName "${sourceFile}" NAME)

    set(destinationDirectory
        "${runtimeDirectory}/${outputDirectory}"
    )

    set(outputFile
        "${destinationDirectory}/${sourceFileName}"
    )
    
    add_custom_command(
        OUTPUT "${outputFile}"

        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${destinationDirectory}"

        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${sourceFile}"
            "${outputFile}"

        DEPENDS "${sourceFile}"
        COMMENT "Copying ${sourceFileName} for ${applicationTarget}"
        VERBATIM
    )

    add_custom_target(
        "${copyTarget}"
        DEPENDS "${outputFile}"
    )

    add_dependencies(
        "${applicationTarget}"
        "${copyTarget}"
    )

endfunction()
