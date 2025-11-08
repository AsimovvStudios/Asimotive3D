#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "a3d.h"
#include "a3d_logging.h"
#include "a3d_window.h"

void a3d_init(a3d* engine, const char* title, int w, int h)
{
	bool init_success = SDL_Init(SDL_INIT_VIDEO);
	if (!init_success) {
		A3D_LOG_ERROR("failed to init SDL: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	engine->window = a3d_create_window(
		title, w, h, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
	);
	if (!engine->window) {
		exit(EXIT_FAILURE);
	}

	engine->running = true;
	/* init event table */
	for (int i = 0; i < SDL_EVENT_LAST; i++) {
		engine->on_event[i] = NULL;
	}
}

void a3d_quit(a3d *engine)
{
	SDL_DestroyWindow(engine->window);
	SDL_Quit();
}
