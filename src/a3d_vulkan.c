#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "a3d.h"
#include "a3d_logging.h"
#include "a3d_vulkan.h"

bool a3d_vk_create_logical_device(a3d* engine)
{
	float priority = 1.0f;
	Uint32 unique_families[2] = {
		engine->vk.graphics_family,
		engine->vk.present_family
	};
	Uint32 n_unique = (engine->vk.present_family == engine->vk.graphics_family) ? 1 : 2;

	/* init queues info */
	VkDeviceQueueCreateInfo queues_info[2];
	for (Uint32 i = 0; i < n_unique; i++) {
		queues_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queues_info[i].pNext = NULL;
		queues_info[i].flags = 0;
		queues_info[i].queueFamilyIndex = unique_families[i];
		queues_info[i].queueCount = 1;
		queues_info[i].pQueuePriorities = &priority;
	}

	VkDeviceCreateInfo device_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = n_unique,
		.pQueueCreateInfos = queues_info,
		.enabledExtensionCount = 0,
		.ppEnabledExtensionNames = NULL
	};

	VkResult result = vkCreateDevice(
		engine->vk.physical, &device_info, NULL, &engine->vk.logical
	);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkCreateDevice failed code: %d", result);
		return false;
	}

	/* retrieve queue handles */
	vkGetDeviceQueue(
		engine->vk.logical, engine->vk.graphics_family,
		0, &engine->vk.graphics_queue
	);
	vkGetDeviceQueue(
		engine->vk.logical, engine->vk.present_family, 0,
		&engine->vk.present_queue
	);

	A3D_LOG_INFO("logical device created");
	A3D_LOG_INFO("    graphics family: %u", engine->vk.graphics_family);
	A3D_LOG_INFO("    present family: %u", engine->vk.present_family);

	return true;
}

bool a3d_vk_init(a3d* engine)
{
	A3D_LOG_INFO("initialising vulkan instance");

	/* get SDL extensions and names */
	Uint32 n_extensions = 0;
	const char* const* extension_names = SDL_Vulkan_GetInstanceExtensions(&n_extensions);
	if (!extension_names) {
		A3D_LOG_ERROR("failed to retrieve vulkan extensions: %s", SDL_GetError());
		return false;
	}

	A3D_LOG_INFO("retrieved %u vulkan extensions from SDL", n_extensions);

	/* output all extension names */
	for (unsigned int i = 0; i < n_extensions; i++)
		A3D_LOG_DEBUG("\text[%u]: %s", i, extension_names[i]);

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
		.enabledExtensionCount = n_extensions,
		.ppEnabledExtensionNames = extension_names
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

	/* init logical device */
	if (!a3d_vk_pick_physical_device(engine)) {
		A3D_LOG_ERROR("failed to pick physical device");
		return false;
	}

	if (!a3d_vk_create_logical_device(engine)) {
		A3D_LOG_ERROR("failed to create logical device");
		return false;
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

bool a3d_vk_pick_physical_device(a3d* engine)
{
	Uint32 n_devices = 0;
	vkEnumeratePhysicalDevices(engine->vk.instance, &n_devices, NULL);
	if (n_devices == 0) {
		A3D_LOG_ERROR("no vulkan-compatible GPU found!");
		return false;
	}

	VkPhysicalDevice devices[16];
	if (n_devices > 16) /* cap it */
		n_devices = 16;
	vkEnumeratePhysicalDevices(engine->vk.instance, &n_devices, devices);

	int best_score = -1;
	VkPhysicalDevice best = VK_NULL_HANDLE;
	for (Uint32 i = 0; i < n_devices; i++) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(devices[i], &properties);

		/* scoring */
		int score = 0;
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 1000;
		else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			score += 500;
		else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
			score += 100;

		if (a3d_vk_pick_queue_families(engine, devices[i]))
			score += 200;

		if (score > best_score) {
			best_score = score;
			best = devices[i];
		}
	}
	if (best == VK_NULL_HANDLE) {
		A3D_LOG_ERROR("failed to find suitable graphics device");
		return false;
	}

	engine->vk.physical = best;

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(best, &properties);
	A3D_LOG_INFO("selected GPU: %s", properties.deviceName);

	return true;
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
	Uint32 graphics_family = UINT32_MAX;
	Uint32 present_family = UINT32_MAX;

	for (Uint32 i = 0; i < n_families; i++) {
		VkBool32 can_present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, engine->vk.surface, &can_present);

		if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
		graphics_family == UINT32_MAX)
			graphics_family = i;

		if (can_present && present_family == UINT32_MAX)
			present_family = i;

		if (graphics_family != UINT32_MAX && present_family != UINT32_MAX)
			break;
	}

	if (graphics_family == UINT32_MAX) {
		A3D_LOG_ERROR("no graphics-capable queue family found");
		return false;
	}

	if (present_family == UINT32_MAX) {
		A3D_LOG_ERROR("no presentation-capable queue family found");
		return false;
	}

	A3D_LOG_INFO("got queue families");
	A3D_LOG_DEBUG("    graphics: %u", graphics_family);
	A3D_LOG_DEBUG("    presentation: %u", present_family);

	engine->vk.graphics_family = graphics_family;
	engine->vk.present_family = present_family;
	return true;
}

void a3d_vk_shutdown(a3d* engine)
{
	if (engine->vk.logical) {
		vkDestroyDevice(engine->vk.logical, NULL);
		engine->vk.logical = VK_NULL_HANDLE;
		A3D_LOG_INFO("vulkan logical device destroyed");
	}

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
