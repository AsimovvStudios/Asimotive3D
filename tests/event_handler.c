#include <stdio.h>

#include <SDL3/SDL.h>

#include "a3d.h"
#include "a3d_logging.h"

#include "event_handler.h"

void assign_events(a3d* engine)
{
	engine->on_event[SDL_EVENT_QUIT] = on_quit;
	engine->on_event[SDL_EVENT_KEY_DOWN] = on_key_down;
}

void on_quit(a3d* engine, const SDL_Event* ev)
{
	(void)ev;
	A3D_LOG_WARN("quitting...");
	engine->running = false;
	a3d_quit(engine);
}

void on_key_down(a3d* engine, const SDL_Event* ev)
{
	(void)engine;
	A3D_LOG_INFO("key pressed: %s\n", SDL_GetKeyName(ev->key.key));
}
