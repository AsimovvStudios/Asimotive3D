#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "a3d.h"
#include "a3d_logging.h"
#include "a3d_window.h"
#include "a3d_renderer.h"
#include "vulkan/a3d_vulkan.h"

void a3d_init(a3d* e, const char* title, int width, int height)
{
	/* zero engine */
	memset(e, 0, sizeof(*e));

	bool init_success = SDL_Init(SDL_INIT_VIDEO);
	if (!init_success) {
		A3D_LOG_ERROR("failed to init SDL: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	e->window = a3d_create_window(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if (!e->window) {
		exit(EXIT_FAILURE);
	}

	/* avoid crash if window is minimised at launch */
	width = height = 0;
	while (width == 0 || height == 0) {
		SDL_GetWindowSize(e->window, &width, &height);
		SDL_Delay(50);
	}

	/* init vulkan */
	if (a3d_vk_init(e)) {
		A3D_LOG_INFO("vulkan initialisation complete");
	}
	else {
		A3D_LOG_ERROR("vulkan initialisation failed");
		SDL_DestroyWindow(e->window);
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	/* init renderer */
	e->renderer = malloc(sizeof *e->renderer);
	if (!e->renderer) {
		A3D_LOG_ERROR("failed to allocate renderer");
		a3d_vk_shutdown(e);
		SDL_DestroyWindow(e->window);
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	if (!a3d_renderer_init(e->renderer)) {
		A3D_LOG_ERROR("renderer initialisation failed");
		free(e->renderer);
		e->renderer = NULL;
		a3d_vk_shutdown(e);
		SDL_DestroyWindow(e->window);
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	e->running = true;
	e->handlers_count = 0;
}

void a3d_quit(a3d *e)
{
	if (e->renderer) {
		a3d_renderer_shutdown(e->renderer);
		free(e->renderer);
		e->renderer = NULL;
	}

	a3d_vk_shutdown(e);
	SDL_DestroyWindow(e->window);
	SDL_Quit();
}
