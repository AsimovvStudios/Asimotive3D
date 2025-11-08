#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "a3d.h"
#include "a3d_logging.h"
#include "a3d_vulkan.h"

bool a3d_vk_init(a3d* engine)
{
	A3D_LOG_INFO("initialising vulkan instance");

	/* get SDL extentions and names */
	Uint32 n_extentions = 0;
	const char* const* extention_names = SDL_Vulkan_GetInstanceExtensions(&n_extentions);
	if (!extention_names) {
		A3D_LOG_ERROR("failed to retrieve vulkan extentions: %s", SDL_GetError());
		return false;
	}

	A3D_LOG_INFO("retrieved %u vulkan extentions from SDL", n_extentions);

	/* output all extention names */
	for (unsigned int i = 0; i < n_extentions; i++)
		A3D_LOG_DEBUG("\text[%u]: %s", i, extention_names[i]);

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
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = n_extentions,
		.ppEnabledExtensionNames = extention_names
	};

	/* create instance */
	VkResult result = vkCreateInstance(&create_info, NULL, &engine->vk.instance);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkCreateInstance failed with code: %d", result);
		engine->vk.instance = VK_NULL_HANDLE;
		return false;
	}
	A3D_LOG_INFO("vulkan instance created");

	/* create window surface */
	bool created_surface = SDL_Vulkan_CreateSurface(
		engine->window, engine->vk.instance, NULL, &engine->vk.surface
	);

	if (!created_surface) {
		A3D_LOG_ERROR("failed to create surface: %s", SDL_GetError());
		vkDestroyInstance(engine->vk.instance, NULL);
		engine->vk.instance = VK_NULL_HANDLE;
		return false;
	}
	else {
		A3D_LOG_INFO("attached SDL vulkan surface");
	}

	return true;
}

void a3d_vk_log_devices(a3d* engine)
{
	Uint32 n_devices = 0;
	vkEnumeratePhysicalDevices(engine->vk.instance, &n_devices, NULL);
	if (n_devices == 0) {
		A3D_LOG_ERROR("no vulkan-compatible GPU found!");
		return;
	}
	fprintf(stdout, "\n");
	A3D_LOG_INFO("found %u devices", n_devices);

	VkPhysicalDevice devices[16];
	if (n_devices > 16) /* cap it */
		n_devices = 16;

	/* fill out devices */
	vkEnumeratePhysicalDevices(engine->vk.instance, &n_devices, devices);
	for (Uint32 i = 0; i < n_devices; i++) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(devices[i], &properties);

		const char* device_type = "UNKNOWN";

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
			device_type = "CPU (Software Renderer)";

		else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			device_type = "GPU (Discrete)";

		else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
			device_type = "GPU (Virtual)";

		else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			device_type = "GPU (Integrated)";

		else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_OTHER)
			device_type = "UNKNOWN (Other)";

		/* logging */
		A3D_LOG_INFO("GPU[%u]: %s", i, properties.deviceName);
		A3D_LOG_DEBUG("    type: %s", device_type);
		A3D_LOG_DEBUG(
			"    api version: %u.%u.%u",
			VK_API_VERSION_MAJOR(properties.apiVersion),
			VK_API_VERSION_MINOR(properties.apiVersion),
			VK_API_VERSION_PATCH(properties.apiVersion)
		);
		A3D_LOG_DEBUG(
			"    driver version: %u.%u.%u",
			VK_API_VERSION_MAJOR(properties.driverVersion),
			VK_API_VERSION_MINOR(properties.driverVersion),
			VK_API_VERSION_PATCH(properties.driverVersion)
		);
		(void)device_type; /* remove unused warning */

		fprintf(stdout, "\n");
		a3d_vk_log_queue_families(engine, devices[i]);
		fprintf(stdout, "\n");
		a3d_vk_pick_queue_families(engine, devices[i]);
	}
}

void a3d_vk_log_queue_families(a3d* engine, VkPhysicalDevice device)
{
	/* query queue families */
	Uint32 n_families = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &n_families, NULL);
	if (n_families == 0) {
		A3D_LOG_ERROR("device has no queue families!");
		return;
	}

	VkQueueFamilyProperties families[32];
	if (n_families > 32)
		n_families = 32; /* cap it */

	vkGetPhysicalDeviceQueueFamilyProperties(device, &n_families, families);

	/* logging */
	A3D_LOG_INFO("found %u queue families", n_families);
	for (Uint32 i = 0; i < n_families; i++) {
		VkQueueFamilyProperties* queue = &families[i];

		A3D_LOG_INFO("queue family[%u]", i);
		A3D_LOG_DEBUG("    queue count: %u:", queue->queueCount);

		A3D_LOG_DEBUG("    flags: %u:", queue->queueCount);
		if (queue->queueFlags & VK_QUEUE_GRAPHICS_BIT)
			A3D_LOG_DEBUG("        GRAPHICS");
		if (queue->queueFlags & VK_QUEUE_COMPUTE_BIT)
			A3D_LOG_DEBUG("        COMPUTE");
		if (queue->queueFlags & VK_QUEUE_TRANSFER_BIT)
			A3D_LOG_DEBUG("        TRANSFER");
		if (queue->queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
			A3D_LOG_DEBUG("        SPARSE_BINDING");
		if (queue->queueFlags & VK_QUEUE_PROTECTED_BIT)
			A3D_LOG_DEBUG("        PROTECTED");

		/* can present to SDL window */
		VkBool32 can_present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, engine->vk.surface, &can_present);
		A3D_LOG_DEBUG("    presentation support: %s", can_present ? "YES" : "NO");
	}
}

bool a3d_vk_pick_queue_families(a3d* engine, VkPhysicalDevice device)
{
	/* query queue families */
	Uint32 n_families = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &n_families, NULL);
	if (n_families == 0) {
		A3D_LOG_ERROR("device has no queue families!");
		return false;
	}

	VkQueueFamilyProperties families[32];
	if (n_families > 32)
		n_families = 32; /* cap it */

	vkGetPhysicalDeviceQueueFamilyProperties(device, &n_families, families);

	/* set sentinel */
	engine->vk.graphics_family = UINT32_MAX;
	engine->vk.present_family = UINT32_MAX;

	for (Uint32 i = 0; i < n_families; i++) {
		VkBool32 can_present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, engine->vk.surface, &can_present);

		bool families_populated =
			(engine->vk.graphics_family != UINT32_MAX) &&
			(engine->vk.present_family != UINT32_MAX);
		if (families_populated)
			break;

		if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
		engine->vk.graphics_family == UINT32_MAX)
			engine->vk.graphics_family = i;

		if (can_present && engine->vk.present_family == UINT32_MAX)
			engine->vk.present_family = i;
	}

	if (engine->vk.graphics_family == UINT32_MAX) {
		A3D_LOG_ERROR("no graphics-capable queue family found");
		return false;
	}

	if (engine->vk.present_family == UINT32_MAX) {
		A3D_LOG_ERROR("no presentation-capable queue family found");
		return false;
	}

	A3D_LOG_INFO("selected queue families");
	A3D_LOG_DEBUG("    graphics: %u", engine->vk.graphics_family);
	A3D_LOG_DEBUG("    presentation: %u", engine->vk.present_family);
	return true;
}

void a3d_vk_shutdown(a3d* engine)
{
	if (engine->vk.surface) {
		vkDestroySurfaceKHR(engine->vk.instance, engine->vk.surface, NULL);
		engine->vk.surface = VK_NULL_HANDLE;
		A3D_LOG_INFO("vulkan surface destroyed");
	}

	if (engine->vk.instance) {
		vkDestroyInstance(engine->vk.instance, NULL);
		engine->vk.instance = VK_NULL_HANDLE;
		A3D_LOG_INFO("vulkan instance destroyed");
	}
}
