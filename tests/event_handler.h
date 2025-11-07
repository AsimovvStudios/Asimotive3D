#pragma once

#include <SDL3/SDL.h>

#include "a3d.h"

void assign_events(a3d* engine);
void on_quit(a3d* engine, const SDL_Event* ev);
void on_key_down(a3d* engine, const SDL_Event* ev);
