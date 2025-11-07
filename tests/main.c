#include <stdbool.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "a3d.h"
#include "event_handler.h"

int main(void)
{
	a3d engine;
	a3d_init(&engine, "test", 800, 600);
	engine.on_event[SDL_EVENT_QUIT] = on_quit;
	engine.on_event[SDL_EVENT_KEY_DOWN] = on_key_down;

	while (engine.running) {
		while(SDL_PollEvent(&engine.ev))
			a3d_handle_events(&engine, &engine.ev);

		SDL_Delay(16); /* 60fps */
	}

	return EXIT_SUCCESS;
}
