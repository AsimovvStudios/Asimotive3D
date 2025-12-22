#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/mat4.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "a3d.h"
#include "a3d_event.h"
#include "a3d_logging.h"
#include "a3d_mesh.h"
#include "a3d_renderer.h"
#include "a3d_transform.h"
#include "vulkan/a3d_vulkan.h"

void on_key_down(a3d* engine, const SDL_Event* ev);

void on_key_down(a3d* engine, const SDL_Event* ev)
{
	(void)engine;
	A3D_LOG_INFO("key pressed: %s", SDL_GetKeyName(ev->key.key));
}

int main(void)
{
	a3d engine;
	if (!a3d_init(&engine, "test", 800, 600)) {
		A3D_LOG_ERROR("engine initialisation failed");
		return EXIT_FAILURE;
	}

	a3d_add_event_handler(&engine, SDL_EVENT_KEY_DOWN, on_key_down);

	/* create a test triangle mesh owned by the app */
	a3d_mesh triangle;
	if (!a3d_init_triangle(&engine, &triangle)) {
		A3D_LOG_ERROR("failed to create triangle mesh");
		a3d_quit(&engine);
		return EXIT_FAILURE;
	}

	/* camera */
	a3d_mvp mvp;
	int w;
	int h;
	SDL_GetWindowSize(engine.window, &w, &h);

	glm_mat4_identity(mvp.model);
	glm_mat4_identity(mvp.view);
	glm_mat4_identity(mvp.proj);

	glm_perspective(glm_rad(70.0f),
		(float)w/(float)h,
		0.1f, 100.0f,
		mvp.proj
	);
	mvp.proj[1][1] *= -1.0f;

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
		if (!engine.running)
			break;

		/* animate clear colour */
		t += 0.016f;
		float r = 0.5f * sinf(t) + 0.5f;
		a3d_vk_set_clear_colour(&engine, r, 0.0f, 0.4f, 1.0f);

		float x = sinf(t) * 2.0f;

		glm_mat4_identity(mvp.model);
		glm_translate(mvp.model, (vec3){x, pow(x, 3)-0.0f, -5.0f});

		/* build render queue for this frame: two triangles at different Z to test depth */
		a3d_renderer_begin_frame(engine.renderer);

		/* closer triangle (z = -4.2) */
		a3d_mvp mvp_close = mvp;
		glm_translate(mvp_close.model, (vec3){0.0f, 0.0f, 0.8f}); /* -5.0 + 0.8 = -4.2 */
		a3d_submit_mesh(&engine, &triangle, &mvp_close);

		/* farther triangle (z = -5.6) */
		a3d_mvp mvp_far = mvp;
		glm_translate(mvp_far.model, (vec3){0.0f, 0.0f, -0.6f}); /* -5.0 - 0.6 = -5.6 */
		a3d_submit_mesh(&engine, &triangle, &mvp_far);

		a3d_renderer_end_frame(engine.renderer);

		a3d_frame(&engine);
		SDL_Delay(16);
	}
	vkDeviceWaitIdle(engine.vk.logical);

	/* cleanup in one place */
	a3d_destroy_mesh(&engine, &triangle);
	a3d_quit(&engine);

	return EXIT_SUCCESS;
}
