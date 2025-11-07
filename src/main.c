#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

bool running = false;

void handle_event(SDL_Event* ev);

void handle_event(SDL_Event* ev)
{
	if (ev->type == SDL_EVENT_QUIT)
		running = false;
}

int main(void)
{
	bool init_success = SDL_Init(SDL_INIT_VIDEO);
	if (!init_success) {
		fprintf(stderr, "failed to init SDL3");
		return EXIT_FAILURE;
	}

	SDL_Window* win = SDL_CreateWindow(
		"Asimotive3D", 800, 600,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
	);
	if (win == NULL) {
		fprintf(stderr, "failed to init SDL3");
		return EXIT_FAILURE;
	}

	SDL_Event ev;
	running = true;
	while (running) {
		/* events */
		while(SDL_PollEvent(&ev))
			handle_event(&ev);

		SDL_Delay(16); /* 60fps */
	}

	SDL_DestroyWindow(win);
	SDL_Quit();
	return EXIT_SUCCESS;
}
