#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "a3d.h"
#include "a3d_event.h"
#include "a3d_logging.h"
#include "a3d_window.h"
#include "a3d_renderer.h"
#include "vulkan/a3d_vulkan.h"

static void a3d_event_on_close_requested(a3d* e, const SDL_Event* ev);
static void a3d_event_on_quit(a3d* e, const SDL_Event* ev);
static void a3d_event_on_resize(a3d* e, const SDL_Event* ev);

void a3d_frame(a3d* e)
{
	if (!e)
		return;

	/* input */
	a3d_pump_events(e);

	if (!e->running)
		return;

	/* handle resize */
	if (e->fb_resized) {
		a3d_vk_recreate_swapchain(e);
		e->fb_resized = false;
	}

	/* render */
	a3d_vk_draw_frame(e);
}

bool a3d_init(a3d* e, const char* title, int width, int height)
{
	/* zero engine */
	memset(e, 0, sizeof(*e));

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		A3D_LOG_ERROR("failed to init SDL: %s", SDL_GetError());
		return false;
	}

	e->window = a3d_create_window(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if (!e->window) {
		A3D_LOG_ERROR("failed to create window");
		SDL_Quit();
		return false;
	}

	/* avoid crash if window is minimised at launch */
	width = height = 0;
	while (width == 0 || height == 0) {
		SDL_GetWindowSize(e->window, &width, &height);
		SDL_Delay(50);
	}

	/* init vulkan */
	if (!a3d_vk_init(e)) {
		A3D_LOG_ERROR("vulkan initialisation failed");
		SDL_DestroyWindow(e->window);
		SDL_Quit();
		return false;
	}

	/* init renderer */
	e->renderer = malloc(sizeof *e->renderer);
	if (!e->renderer) {
		A3D_LOG_ERROR("failed to allocate renderer");
		a3d_vk_shutdown(e);
		SDL_DestroyWindow(e->window);
		SDL_Quit();
		return false;
	}

	if (!a3d_renderer_init(e->renderer)) {
		A3D_LOG_ERROR("renderer initialisation failed");
		free(e->renderer);
		e->renderer = NULL;
		a3d_vk_shutdown(e);
		SDL_DestroyWindow(e->window);
		SDL_Quit();
		return false;
	}

	e->running = true;
	e->handlers_count = 0;

	/* sane defaults */
	a3d_add_event_handler(e, SDL_EVENT_QUIT, a3d_event_on_quit);
	a3d_add_event_handler(e, SDL_EVENT_WINDOW_CLOSE_REQUESTED, a3d_event_on_close_requested);
	a3d_add_event_handler(e, SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, a3d_event_on_resize);

	return true;
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

bool a3d_submit_mesh(a3d* e, const a3d_mesh* mesh, const a3d_mvp* mvp)
{
	if (!e || !e->renderer)
		return false;

	return a3d_renderer_draw_mesh(e->renderer, mesh, mvp);
}

static void a3d_event_on_quit(a3d* e, const SDL_Event* ev)
{
	(void)ev;
	e->running = false;
}

static void a3d_event_on_close_requested(a3d* e, const SDL_Event* ev)
{
	(void)ev;
	e->running = false;
}

static void a3d_event_on_resize(a3d* e, const SDL_Event* ev)
{
	(void)ev;
	e->fb_resized = true;
}