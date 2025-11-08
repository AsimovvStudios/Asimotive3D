#include <stdio.h>

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "a3d.h"
#include "a3d_logging.h"
#include "a3d_vulkan.h"

bool a3d_vk_init(a3d* engine)
{
	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,

		.pApplicationName = "Asimotive3d",
		.applicationVersion = VK_MAKE_VERSION(0, 1, 0),

		.pEngineName = "Asimotive3d",
		.engineVersion = VK_MAKE_VERSION(0, 1, 0),

		.apiVersion = VK_API_VERSION_1_3
	};

	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info
	};

	VkResult result = vkCreateInstance(&create_info, NULL, &engine->vk.instance);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkCreateInstance failed with code: %d", result);
		engine->vk.instance = VK_NULL_HANDLE;
		return false;
	}
	else {
		A3D_LOG_INFO("vulkan instance created");
		return true;
	}
}

void a3d_vk_shutdown(a3d* engine)
{
	if (engine->vk.instance != VK_NULL_HANDLE) {
		vkDestroyInstance(engine->vk.instance, NULL);
		engine->vk.instance = VK_NULL_HANDLE;
		A3D_LOG_INFO("vulkan instance destroyed");
	}
}
