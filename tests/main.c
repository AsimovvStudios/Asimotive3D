#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "a3d.h"
#include "a3d_logging.h"
#include "vulkan/a3d_vulkan.h"

void on_quit(a3d* engine, const SDL_Event* ev);
void assign_events(a3d* engine);

void on_quit(a3d* engine, const SDL_Event* ev)
{
	A3D_LOG("\n\n\n");
	(void)ev;
	A3D_LOG_INFO("quitting");
	engine->running = false;
	a3d_quit(engine);
}

void on_key_down(a3d* engine, const SDL_Event* ev)
{
	(void)engine;
	A3D_LOG_INFO("key pressed: %s", SDL_GetKeyName(ev->key.key));
}

int main(void)
{
    a3d engine;
    a3d_init(&engine, "test", 800, 600);
    engine.on_event[SDL_EVENT_QUIT] = on_quit;
    engine.on_event[SDL_EVENT_KEY_DOWN] = on_key_down;

	float t = 0.0f;

	A3D_LOG();
	A3D_LOG("TEST LOG");
	A3D_LOG_INFO("TEST INFO");
	A3D_LOG_DEBUG("TEST DEBUG");
	A3D_LOG_ERROR("TEST ERROR");

	A3D_LOG();
#if A3D_VK_VALIDATION
	A3D_LOG_INFO("CREATING DEBUG");
#else
	A3D_LOG_ERROR("NO DEBUG");
#endif
	A3D_LOG();

    for (;;) {
        while(SDL_PollEvent(&engine.ev))
            a3d_handle_events(&engine, &engine.ev);

		if (!engine.running)
			break;

		t += 0.016f;
		float r = 0.5f * sin(t) + 0.5f;
		a3d_vk_set_clear_colour(&engine, r, 0.0f, 0.4f, 1.0f);

		a3d_vk_draw_frame(&engine);
        SDL_Delay(16);
    }
    return EXIT_SUCCESS;
}
