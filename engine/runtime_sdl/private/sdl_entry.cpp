#define SDL_MAIN_USE_CALLBACKS

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>

#include <render_sdl/public/graphics_context.h>
#include <render_sdl/public/renderer.h>

#include <sdl_support/public/validation.h>

#include <runtime/public/runtime.h>

#include <input_sdl/public/input_backend.h>

namespace {

struct EngineState {
    void* gameState{};

    SDL_Window* window{};
    std::optional<engine::graphics::GraphicsContext> graphicsContext;

    std::optional<engine::graphics::Renderer> renderer;

    Uint64 previousFrameTime{};

    bool gameStartAttempted{};

    engine::input::InputBackend inputBackend{};
    engine::Engine engine{inputBackend.get()};
};

SDL_AppResult toSDLResult(engine::Result result) {
    switch (result) {
    case engine::Result::Continue:
        return SDL_APP_CONTINUE;

    case engine::Result::Success:
        return SDL_APP_SUCCESS;

    case engine::Result::Failure:
        return SDL_APP_FAILURE;
    }

    return SDL_APP_FAILURE;
}

engine::Result toEngineResult(SDL_AppResult result) {
    switch (result) {
    case SDL_APP_CONTINUE:
        return engine::Result::Continue;

    case SDL_APP_SUCCESS:
        return engine::Result::Success;

    case SDL_APP_FAILURE:
        return engine::Result::Failure;
    }

    return engine::Result::Failure;
}

using namespace engine::graphics;

} // namespace

SDL_AppResult SDL_AppInit(void** rawEngineState, int argc, char* argv[]) {
    // Create Game Desctiption
    const engine::GameDescription game = engine::createGame();

    // Create Engine State
    auto* engineState = new EngineState{};
    *rawEngineState = engineState;

    // Init SDL systems
    if (!validate(SDL_Init(SDL_INIT_VIDEO), "Init SDL error")) {
        return SDL_APP_FAILURE;
    }

    // Create Main Window
    engineState->window = SDL_CreateWindow(game.title, game.windowWidth, game.windowHeight, SDL_WINDOW_RESIZABLE);
    if (!validate(engineState->window, "Create Main Window error")) {
        return SDL_APP_FAILURE;
    }

    // Create graphics context
    engineState->graphicsContext = GraphicsContext::create(engineState->window);
    if (!engineState->graphicsContext) {
        return SDL_APP_FAILURE;
    }

    // Create renderer
    engineState->renderer = Renderer::create(engineState->window, &engineState->graphicsContext.value());
    if (!engineState->renderer) {
        return SDL_APP_FAILURE;
    }

    // Start Game
    engineState->gameStartAttempted = true;
    auto startGameResult = toSDLResult(engine::startGame(&engineState->gameState));
    if (startGameResult == SDL_APP_CONTINUE) {
        engineState->previousFrameTime = SDL_GetTicksNS();
        return SDL_APP_CONTINUE;
    }

    return startGameResult;
}

SDL_AppResult SDL_AppIterate(void* rawEngineState) {
    // Get Engine State
    auto& engineState = *static_cast<EngineState*>(rawEngineState);

    // Update time
    const Uint64 currentTime = SDL_GetTicksNS();
    double deltaSeconds = static_cast<double>(currentTime - engineState.previousFrameTime) / 1'000'000'000.0;
    engineState.previousFrameTime = currentTime;

    // Create Frame Info
    const engine::FrameInfo frame{.realDeltaSeconds = deltaSeconds};

    // Update logic
    const engine::Result updateResult = engine::updateGame(engineState.gameState, engineState.engine, frame);
    if (updateResult != engine::Result::Continue) {
        return toSDLResult(updateResult);
    }

    // Render Frame
    engine::RenderFrameData renderFrameData{};
    const engine::Result renderResult = engine::renderGame(engineState.gameState, renderFrameData);
    if (renderResult != engine::Result::Continue) {
        return toSDLResult(renderResult);
    }

    // Update renderer
    if (!engineState.renderer.value().renderFrame(renderFrameData)) {
        return SDL_APP_FAILURE;
    }

    // Clear input
    engineState.inputBackend.endFrame();

    return SDL_APP_CONTINUE;
}

engine::Event translateEvent(const SDL_Event& sdlEvent) {
    switch (sdlEvent.type) {
    case SDL_EVENT_QUIT:
        return {.type = engine::EventType::CloseRequested};

    default:
        return {.type = engine::EventType::Unknown};
    }
}

SDL_AppResult SDL_AppEvent(void* rawEngineState, SDL_Event* sdlEvent) {
    // Get Engine State
    auto& engineState = *static_cast<EngineState*>(rawEngineState);

    switch (sdlEvent->type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        engineState.inputBackend.process(sdlEvent->key);
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        engineState.inputBackend.process(sdlEvent->button);
        break;

    case SDL_EVENT_MOUSE_MOTION:
        engineState.inputBackend.process(sdlEvent->motion);
        break;

    case SDL_EVENT_MOUSE_WHEEL:
        engineState.inputBackend.process(sdlEvent->wheel);
        break;

    case SDL_EVENT_WINDOW_FOCUS_LOST:
        engineState.inputBackend.cancel();
        break;

    default:
        break;
    }

    const engine::Event event = translateEvent(*sdlEvent);
    return toSDLResult(engine::processEvent(engineState.gameState, event));
}

void SDL_AppQuit(void* rawEngineState, SDL_AppResult sdlResult) {
    // Get Engine State
    auto* engineState = static_cast<EngineState*>(rawEngineState);
    if (!engineState) {
        return;
    }

    // Stop Game
    if (engineState->gameStartAttempted) {
        engine::stopGame(engineState->gameState, toEngineResult(sdlResult));
    }

    // Release renderer
    if (engineState->renderer) {
        engineState->renderer.reset();
    }

    // Release graphics context
    if (engineState->graphicsContext) {
        engineState->graphicsContext.reset();
    }

    // Destroy Main Window
    if (engineState->window) {
        SDL_DestroyWindow(engineState->window);
    }

    // Delete Engine State
    delete engineState;
}
