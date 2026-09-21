#pragma once

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_error.h>

inline bool validate(bool value, const char* context) {
    if (!value) {
        SDL_Log("%s: %s", context, SDL_GetError());
        return false;
    }
    return true;
}
inline bool validate(void* ptr, const char* context) {
    if (!ptr) {
        SDL_Log("%s: %s", context, SDL_GetError());
        return false;
    }
    return true;
}
