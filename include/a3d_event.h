#pragma once

#include <SDL3/SDL.h>

#include "a3d.h"

const char* a3d_sdl_event_to_str(SDL_EventType type);
void a3d_handle_events(a3d* engine, const SDL_Event* ev);
