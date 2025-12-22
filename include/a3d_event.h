#pragma once

#include <SDL3/SDL.h>

#include "a3d.h"

const char* a3d_sdl_event_to_str(SDL_EventType type);
void a3d_handle_events(a3d* e, const SDL_Event* ev);
bool a3d_add_event_handler(a3d* e, Uint32 type, a3d_event_handler fn);
void a3d_pump_events(a3d* e);
