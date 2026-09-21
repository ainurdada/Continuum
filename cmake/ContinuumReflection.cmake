function(continuum_enable_reflection targetName)
    cmake_parse_arguments(PARSE_ARGV 1 REFLECTION "" MODULE ROOTS)

    set(REFLECTION_HEADERS "")

    foreach(root IN LISTS REFLECTION_ROOTS)
        file(GLOB_RECURSE rootHeaders CONFIGURE_DEPENDS LIST_DIRECTORIES false "${root}/*.h")
        list(APPEND REFLECTION_HEADERS ${rootHeaders})
    endforeach()

    list(REMOVE_DUPLICATES REFLECTION_HEADERS)
    list(SORT REFLECTION_HEADERS)

    list(JOIN REFLECTION_HEADERS "\n" manifestContent)

    set(manifestPath "${CMAKE_CURRENT_BINARY_DIR}/${REFLECTION_MODULE}.headers.txt")

    file(CONFIGURE OUTPUT "${manifestPath}" CONTENT "${manifestContent}" NEWLINE_STYLE UNIX @ONLY)

    set(reflectionInputPath "${CMAKE_CURRENT_BINARY_DIR}/${REFLECTION_MODULE}.reflection_input.cpp")

    file(CONFIGURE OUTPUT "${reflectionInputPath}" CONTENT "")

    target_sources(${targetName} PRIVATE "${reflectionInputPath}")

    set(generatedSourcePath "${CMAKE_CURRENT_BINARY_DIR}/${REFLECTION_MODULE}.reflection.generated.cpp")
    set(generatedEcsSourcePath "${CMAKE_CURRENT_BINARY_DIR}/${REFLECTION_MODULE}.ecs_reflection.generated.cpp")
    set(generatedEditorSourcePath "${CMAKE_CURRENT_BINARY_DIR}/${REFLECTION_MODULE}.editor_reflection.generated.cpp")
    set(generatedStampPath "${CMAKE_CURRENT_BINARY_DIR}/${REFLECTION_MODULE}.reflection.stamp")

    add_custom_command(
        OUTPUT "${generatedStampPath}"
        BYPRODUCTS "${generatedSourcePath}" "${generatedEcsSourcePath}" "${generatedEditorSourcePath}"

        COMMAND
        $<TARGET_FILE:ContinuumReflectionGenerator>
        --manifest
        "${manifestPath}"
        "${generatedSourcePath}"
        "${generatedEcsSourcePath}"
        "${REFLECTION_MODULE}"
        --compdb
        "${CMAKE_BINARY_DIR}"

        COMMAND
        ${CMAKE_COMMAND}
        -E
        touch
        "${generatedStampPath}"

        DEPENDS
        ContinuumReflectionGenerator
        "${manifestPath}"
        "${REFLECTION_HEADERS}"
        "${reflectionInputPath}"
        "${CMAKE_BINARY_DIR}/compile_commands.json"

        VERBATIM
    )

    set(generatedReflectionTarget "${targetName}Reflection")
    add_custom_target("${generatedReflectionTarget}" DEPENDS "${generatedStampPath}")

    add_dependencies(
        "${targetName}"
        "${generatedReflectionTarget}"
    )

    target_sources(${targetName} PRIVATE "${generatedSourcePath}")

    set(generatedEcsReflectionTarget "${targetName}EcsReflection")
    add_library(
        "${generatedEcsReflectionTarget}"
        OBJECT
        "${generatedEcsSourcePath}"
    )

    target_link_libraries(
        "${generatedEcsReflectionTarget}"
        PRIVATE
        "${targetName}"
        ContinuumEngine_EcsReflection
    )

    set_property(TARGET "${targetName}" PROPERTY CONTINUUM_ECS_REFLECTION_TARGET "${generatedEcsReflectionTarget}")
    set_property(TARGET "${targetName}" PROPERTY CONTINUUM_REFLECTION_MODULE_NAME "${REFLECTION_MODULE}")
    set_property(TARGET "${targetName}" PROPERTY CONTINUUM_EDITOR_REFLECTION_SOURCE "${generatedEditorSourcePath}")

    add_dependencies(
        "${generatedEcsReflectionTarget}"
        "${generatedReflectionTarget}"
    )

endfunction()

function(continuum_link_modules consumerTarget)
    cmake_parse_arguments(PARSE_ARGV 1 LINK "" "" MODULES)

    get_property(hasReflectionModules TARGET "${consumerTarget}" PROPERTY CONTINUUM_REFLECTION_MODULES SET)
    if(hasReflectionModules)
        get_target_property(reflectionTarget "${consumerTarget}" CONTINUUM_REFLECTION_MODULES)
        set(reflectionModules "${reflectionTarget}")
    else()
        set(reflectionModules "")
    endif()

    get_property(hasEditorReflectionSources TARGET "${consumerTarget}" PROPERTY CONTINUUM_EDITOR_REFLECTION_SOURCES SET)
    if(hasEditorReflectionSources)
        get_target_property(editorReflectionSourcesFromPoperty "${consumerTarget}" CONTINUUM_EDITOR_REFLECTION_SOURCES)
        set(editorReflectionSources "${editorReflectionSourcesFromPoperty}")
    else()
        set(editorReflectionSources "")
    endif()

    foreach(module IN LISTS LINK_MODULES)
        target_link_libraries("${consumerTarget}" PRIVATE "${module}")
        get_target_property(ecsReflectionTarget "${module}" CONTINUUM_ECS_REFLECTION_TARGET)
        if(TARGET "${ecsReflectionTarget}")
            target_link_libraries("${consumerTarget}" PRIVATE "${ecsReflectionTarget}" ContinuumEngine_EcsReflection)
        endif()

        get_property(hasReflectionModuleName TARGET "${module}" PROPERTY CONTINUUM_REFLECTION_MODULE_NAME SET)
        if(hasReflectionModuleName)
            get_target_property(reflectionModuleName "${module}" CONTINUUM_REFLECTION_MODULE_NAME)
            list(APPEND reflectionModules "${reflectionModuleName}")
        endif()

        get_property(hasEditorReflectionSource TARGET "${module}" PROPERTY CONTINUUM_EDITOR_REFLECTION_SOURCE SET)
        if(hasEditorReflectionSource)
            get_target_property(editorReflectionSource "${module}" CONTINUUM_EDITOR_REFLECTION_SOURCE)
            list(APPEND editorReflectionSources "${editorReflectionSource}")
        endif()
    endforeach()

    list(REMOVE_DUPLICATES reflectionModules)
    set_target_properties("${consumerTarget}" PROPERTIES CONTINUUM_REFLECTION_MODULES "${reflectionModules}")

    list(REMOVE_DUPLICATES editorReflectionSources)
    set_target_properties("${consumerTarget}" PROPERTIES CONTINUUM_EDITOR_REFLECTION_SOURCES "${editorReflectionSources}")
endfunction()

function(continuum_finalize_reflection consumerTarget)
    cmake_parse_arguments(PARSE_ARGV 1 FINALIZE EDITOR "" "")
    if(FINALIZE_EDITOR)
        get_property(hasEditorReflectionSources TARGET "${consumerTarget}" PROPERTY CONTINUUM_EDITOR_REFLECTION_SOURCES SET)
        if(hasEditorReflectionSources)
            get_target_property(editorReflectionSources "${consumerTarget}" CONTINUUM_EDITOR_REFLECTION_SOURCES)
            target_sources(
                "${consumerTarget}"
                PRIVATE
                ${editorReflectionSources}
            )
        endif()
    endif()


    get_property(hasReflectionModules TARGET "${consumerTarget}" PROPERTY CONTINUUM_REFLECTION_MODULES SET)
    if(hasReflectionModules)
        get_target_property(reflectionTarget "${consumerTarget}" CONTINUUM_REFLECTION_MODULES)
        set(reflectionModules "${reflectionTarget}")
    else()
        set(reflectionModules "")
    endif()

    list(JOIN reflectionModules "\n" reflectionModulesFileContent)

    set(manifestPath "${CMAKE_CURRENT_BINARY_DIR}/${consumerTarget}.reflection_modules.txt")

    file(CONFIGURE
        OUTPUT "${manifestPath}"
        CONTENT "${reflectionModulesFileContent}"
        NEWLINE_STYLE
        UNIX
        @ONLY
    )

    set(generatedSourcePath "${CMAKE_CURRENT_BINARY_DIR}/${consumerTarget}.reflection_bootstrap.generated.cpp")
    set(generatedEcsSourcePath "${CMAKE_CURRENT_BINARY_DIR}/${consumerTarget}.ecs_reflection_bootstrap.generated.cpp")
    set(generatedStampPath "${CMAKE_CURRENT_BINARY_DIR}/${consumerTarget}.reflection_bootstrap.stamp")
    
    set(generatedEditorPath "")
    if(FINALIZE_EDITOR)
        list(APPEND generatedEditorPath "${CMAKE_CURRENT_BINARY_DIR}/${consumerTarget}.editor_reflection_bootstrap.generated.cpp")
    endif()
    

    add_custom_command(
        OUTPUT "${generatedStampPath}"
        BYPRODUCTS "${generatedSourcePath}" "${generatedEcsSourcePath}" ${generatedEditorPath}

        COMMAND
        $<TARGET_FILE:ContinuumReflectionGenerator>
        --bootstrap
        "${manifestPath}"
        "${generatedSourcePath}"
        "${generatedEcsSourcePath}"
        ${generatedEditorPath}

        COMMAND
        ${CMAKE_COMMAND}
        -E
        touch
        "${generatedStampPath}"

        DEPENDS
        ContinuumReflectionGenerator
        "${manifestPath}"

        VERBATIM
    )

    add_custom_target(
        "${consumerTarget}ReflectionBootstrap"
        DEPENDS
        "${generatedStampPath}"
    )

    add_dependencies("${consumerTarget}" "${consumerTarget}ReflectionBootstrap")

    target_sources(
        "${consumerTarget}"
        PRIVATE
        "${generatedSourcePath}"
        "${generatedEcsSourcePath}"
        ${generatedEditorPath}
    )
endfunction()
