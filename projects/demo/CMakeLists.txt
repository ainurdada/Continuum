file(GLOB_RECURSE HEADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/code/public/*.h"
)

file(GLOB_RECURSE SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/code/private/*.cpp"
)

add_executable(
    ContinuumDemo

    main.cpp
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

continuum_link_modules(
    ContinuumDemo

    MODULES
    ContinuumEngine
    ContinuumEngine_Scene
    ContinuumEngine_RuntimeSdl
)

continuum_finalize_reflection(ContinuumDemo)

target_include_directories(
    ContinuumDemo
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/code/public
)

continuum_configure_runtime(ContinuumDemo)
continuum_copy_runtime_file(
    ContinuumDemo
    ContinuumDemoScenes
    "${CMAKE_CURRENT_SOURCE_DIR}/scenes/main.cscn"
    "scenes"
)
