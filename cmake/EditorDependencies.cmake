include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
    GIT_TAG v1.92.9b-docking
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

set(IMGUIZMO_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY "https://github.com/CedricGuillemet/ImGuizmo.git"
    GIT_TAG 1.10
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(
    imgui
    imguizmo
)

add_library(
    ContinuumImGui
    STATIC

    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp

    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp

    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp
)

target_include_directories(
    ContinuumImGui
    PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(
    ContinuumImGui
    PUBLIC
    SDL3::SDL3
)

target_link_libraries(
    imguizmo
    PUBLIC
    ContinuumImGui
)
