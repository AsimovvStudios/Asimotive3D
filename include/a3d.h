#pragma once

#include <stdbool.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

/* logging */
#ifdef DEBUG
	#define A3D_LOG_DEBUG(fmt, ...) fprintf(stdout, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
	#define A3D_LOG_DEBUG(fmt, ...) ((void)0)
#endif

/* structures */
typedef struct a3d a3d;
typedef void (*a3d_event_handler)(a3d *engine, const SDL_Event *e);

struct a3d {
	/* SDL */
	SDL_Window* window;

	/* loop */
	SDL_Event   ev;
	a3d_event_handler on_event[SDL_EVENT_LAST];
	bool        running;
};

/* declarations */
void a3d_handle_events(a3d* engine, const SDL_Event* ev);
void a3d_init(a3d *engine, const char* title, int w, int h);
void a3d_quit(a3d *engine);
