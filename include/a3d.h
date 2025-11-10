#pragma once

#include <stdbool.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#if !defined(A3D_VK_VALIDATION)
#	ifndef NDEBUG
#		define A3D_VK_VALIDATION 1
#	else
#		define A3D_VK_VALIDATION 0
#	endif
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

	/* vulkan & graphics */
	struct {
		VkInstance instance;
		VkSurfaceKHR surface;
		VkDebugUtilsMessengerEXT debug_messenger;

		Uint32  graphics_family;
		Uint32  present_family;
		VkQueue graphics_queue;
		VkQueue present_queue;

		VkDevice logical;
		VkPhysicalDevice physical;

		VkSwapchainKHR swapchain;
		VkFormat swapchain_format;
		VkExtent2D swapchain_extent;
		VkImage  swapchain_images[8];
		VkImageView swapchain_views[8];
		Uint32   n_swapchain_images;

		VkRenderPass render_pass;
		VkFramebuffer framebuffers[8];
		VkClearValue clear_colour;

		VkCommandPool command_pool;
		VkCommandBuffer command_buffers[8];

		VkSemaphore image_available;
		VkSemaphore render_finished;
		VkFence in_flight;
	} vk;
};

/* declarations */
void a3d_handle_events(a3d* engine, const SDL_Event* ev);
void a3d_init(a3d *engine, const char* title, int w, int h);
void a3d_quit(a3d *engine);
