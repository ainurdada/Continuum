<img width="2138" height="736" alt="continuum_label" src="https://github.com/user-attachments/assets/428b4ef5-1cd7-449e-8a17-aa095b802f18" />

# Continuum

Continuum is a C++23 game engine in development. It has an archetype-based ECS, generated reflection, an SDL GPU renderer, and an editor for building and playing scenes. The included demo project shows the current workflow; the renderer, asset pipeline, and game features are still growing.

## Dependencies

- **Build:** CMake 4.3+, Ninja, and a C++23 compiler. On macOS, install Xcode Command Line Tools; on Windows, install the Visual Studio C++ build tools.
- **Engine:** SDL3 3.4.14, GLM 1.0.3, and nlohmann/json 3.12.0.
- **Editor and assets:** Dear ImGui 1.92.9b (docking), ImGuizmo 1.10, and Assimp 6.0.5.
- **Reflection and shaders:** LLVM/Clang 22.1.8 and SDL_shadercross 3.0.0.

CMake downloads the source dependencies and platform-specific Assimp and LLVM packages during configuration. SDL_shadercross is included in the repository. The current build supports **macOS arm64** and **Windows x64**.

## Build and run

```sh
git clone https://github.com/ainurdada/Continuum.git
cd Continuum
cmake --preset dev
cmake --build --preset dev
```

Open the included project in the editor:

```sh
# macOS
./build/dev/editor/Debug/ContinuumEditor projects/demo/ContinuumDemo.continuum

# Windows (PowerShell)
.\build\dev\editor\Debug\ContinuumEditor.exe .\projects\demo\ContinuumDemo.continuum
```

The standalone demo executable is in `build/dev/projects/demo/Debug/`. See the [roadmap](docs/ROADMAP.md) for what's implemented and what's next.
