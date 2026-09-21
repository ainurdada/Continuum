function(set_module_defaults module_name module_type)
    if(${module_type} STREQUAL "STATIC")
        target_include_directories(
            ${module_name}
            PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
        )

        target_include_directories(
            ${module_name}
            PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
        )

    elseif(${module_type} STREQUAL "INTERFACE")
        target_include_directories(
            ${module_name}
            INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
        )
    endif()

target_link_libraries(
    ContinuumEngine
    INTERFACE
    ${module_name}
)

endfunction()
