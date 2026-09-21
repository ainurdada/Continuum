add_library(
    ContinuumEngine
    INTERFACE
)

add_subdirectory(ecs)
add_subdirectory(reflection)
add_subdirectory(reflection_json)
add_subdirectory(ecs_reflection)
add_subdirectory(math)
add_subdirectory(input)
add_subdirectory(render)
add_subdirectory(scene)
add_subdirectory(sdl_support)
add_subdirectory(render_sdl)
add_subdirectory(input_sdl)
