file(GLOB_RECURSE HEADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/code/public/*.h"
)

file(GLOB_RECURSE SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/code/private/*.cpp"
)

add_library(
    ContinuumDemoGame
    STATIC
    ${HEADER_FILES}
    ${SOURCE_FILES}
)

target_include_directories(
    ContinuumDemoGame
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/code/public
)

target_link_libraries(
    ContinuumDemoGame
    PUBLIC
    ContinuumEngine
)

target_link_libraries(
    ContinuumEditor
    PUBLIC
    ContinuumDemoGame
)

add_executable(
    ContinuumDemo

    main.cpp
)

continuum_link_modules(
    ContinuumDemo

    MODULES
    ContinuumDemoGame
    ContinuumEngine
    ContinuumEngine_Scene
    ContinuumEngine_RuntimeSdl
)

continuum_finalize_reflection(ContinuumDemo)

continuum_configure_runtime(ContinuumDemo)
continuum_copy_runtime_file(
    ContinuumDemo
    ContinuumDemoScenes
    "${CMAKE_CURRENT_SOURCE_DIR}/scenes/main.cscn"
    "scenes"
)
