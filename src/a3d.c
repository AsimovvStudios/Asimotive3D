#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "a3d.h"

void a3d_init(a3d* engine, const char* title, int w, int h)
{
	bool init_success = SDL_Init(SDL_INIT_VIDEO);
	if (!init_success) {
		SDL_Log("failed to init SDL: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	engine->window = SDL_CreateWindow(
			title, w, h, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
			);
	if (engine->window == NULL) {
		SDL_Log("failed to create window: %s", SDL_GetError());
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
