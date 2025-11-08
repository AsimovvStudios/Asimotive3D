#pragma once

#include <SDL3/SDL.h>

SDL_Window* a3d_create_window(
	const char* title, int w, int h, SDL_WindowFlags flags
);
