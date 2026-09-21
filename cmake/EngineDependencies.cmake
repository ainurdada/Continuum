include(FetchContent)

set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3
    URL "https://github.com/libsdl-org/SDL/releases/download/release-3.4.14/SDL3-3.4.14.tar.gz"
    URL_HASH "SHA256=30d4aa2b3037718142b32dffd4e72f917ebb6cc5227150e7bb9c45efb2153aeb"
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(
    glm
    URL "https://github.com/g-truc/glm/archive/refs/tags/1.0.3.tar.gz"
    URL_HASH "SHA256=6775e47231a446fd086d660ecc18bcd076531cfedd912fbd66e576b118607001"
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(
    json
    URL "https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz"
    URL_HASH "SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa"
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(
    SDL3
    glm
    json
)