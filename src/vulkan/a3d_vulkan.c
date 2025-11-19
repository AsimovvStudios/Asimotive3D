#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "a3d.h"
#include "a3d_logging.h"
#include "a3d_mesh.h"
#include "a3d_renderer.h"
#include "vulkan/a3d_vulkan.h"
#include "vulkan/a3d_vulkan_pipeline.h"

#if A3D_VK_VALIDATION
#include "vulkan/a3d_vulkan_debug.h"
static const char* const validation_layers[] = {
	"VK_LAYER_KHRONOS_validation"
};
#endif

static VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR* capabilities, SDL_Window* window);
static VkSurfaceFormatKHR choose_surface_format( const VkSurfaceFormatKHR* formats, Uint32 n_formats);
static VkPresentModeKHR choose_present_mode(const VkPresentModeKHR* modes, Uint32 n_modes);

bool a3d_vk_allocate_command_buffers(a3d* engine)
{
	A3D_LOG_INFO("allocating command buffers");

	VkCommandBufferAllocateInfo info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = engine->vk.command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = engine->vk.n_swapchain_images
	};

	VkResult result = vkAllocateCommandBuffers(engine->vk.logical, &info, engine->vk.command_buffers);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("failed to allocate command buffers with code %d", result);
		return false;
	}

	A3D_LOG_INFO("allocated %u command buffers", engine->vk.n_swapchain_images);
	return true;
}

bool a3d_vk_create_command_pool(a3d* engine)
{
	A3D_LOG_INFO("creating command pool");

	VkCommandPoolCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = engine->vk.graphics_family,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
	};

	VkResult result = vkCreateCommandPool(engine->vk.logical, &info, NULL, &engine->vk.command_pool);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("failed to create command pool with code %d", result);
		return false;
	}

	A3D_LOG_INFO("created command pool");
	return true;
}

bool a3d_vk_create_framebuffers(a3d* engine)
{
	A3D_LOG_INFO("creating framebuffers");

	for (Uint32 i = 0; i < engine->vk.n_swapchain_images; i++) {
		VkImageView attachments[] = {engine->vk.swapchain_views[i]};

		VkFramebufferCreateInfo info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = engine->vk.render_pass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = engine->vk.swapchain_extent.width,
			.height = engine->vk.swapchain_extent.height,
			.layers = 1
		};

		VkResult result = vkCreateFramebuffer(engine->vk.logical, &info, NULL, &engine->vk.framebuffers[i]);
		if (result != VK_SUCCESS) {
			A3D_LOG_ERROR("failed to create framebuffer %u with code %d", i, result);
			return false;
		}
		A3D_LOG_INFO("created framebuffer %u", i);
	}

	return true;
}

bool a3d_vk_create_image_views(a3d* engine)
{
	A3D_LOG_INFO("creating image views for swapchain");

	for (Uint32 i = 0; i < engine->vk.n_swapchain_images; i++) {
		VkImageViewCreateInfo info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = engine->vk.swapchain_images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = engine->vk.swapchain_format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
		};

		VkResult result = vkCreateImageView(engine->vk.logical, &info, NULL, &engine->vk.swapchain_views[i]);
		if (result != VK_SUCCESS) {
			A3D_LOG_ERROR("failed to create image view %u with code %d", i, result);
			return false;
		}
		A3D_LOG_DEBUG("created image view %u", i);
	}
	A3D_LOG_INFO("all %u image views created", engine->vk.n_swapchain_images);
	return true;
}

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

	Uint32 n_device_extensions = 1;
	const char* device_extensions[] = {"VK_KHR_swapchain"};
	VkDeviceCreateInfo device_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = n_unique,
		.pQueueCreateInfos = queues_info,
		.enabledExtensionCount = n_device_extensions,
		.ppEnabledExtensionNames = device_extensions
	};

	VkResult result = vkCreateDevice(
		engine->vk.physical, &device_info, NULL, &engine->vk.logical
	);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkCreateDevice failed with code: %d", result);
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

bool a3d_vk_create_render_pass(a3d* engine)
{
	A3D_LOG_INFO("creating render pass");

	VkAttachmentDescription colour_attach = {
		.format = engine->vk.swapchain_format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference colour_reference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colour_reference
	};

	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	};

	VkRenderPassCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colour_attach,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	VkResult result = vkCreateRenderPass(engine->vk.logical, &info, NULL, &engine->vk.render_pass);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("failed to create render pass with code %d", result);
		return false;
	}
	A3D_LOG_INFO("created render pass");
	return true;
}

bool a3d_vk_create_swapchain(a3d* engine)
{
	VkPhysicalDevice device = engine->vk.physical;
	VkSurfaceKHR surface = engine->vk.surface;

	/* querying */
	VkSurfaceCapabilitiesKHR capabilities; /* capabilities */
	VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("failed to get surface capabilities with code %d", result);
		return false;
	}

	Uint32 n_formats = 0; /* formats */
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &n_formats, NULL);
	VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * n_formats);
	if (formats == NULL) {
		A3D_LOG_ERROR("ran out of memory! can't allocate memory for formats");
		return false;
	}
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &n_formats, formats);

	Uint32 n_modes = 0; /* present modes */
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &n_modes, NULL);
	VkPresentModeKHR* modes = malloc(sizeof(VkPresentModeKHR) * n_modes);
	if (modes == NULL) {
		A3D_LOG_ERROR("ran out of memory! can't allocate memory for modes");
		free(formats);
		return false;
	}
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &n_modes, modes);

	/* choose best */
	VkSurfaceFormatKHR best_format = choose_surface_format(formats, n_formats);
	VkPresentModeKHR best_mode = choose_present_mode(modes, n_modes);
	VkExtent2D extent = choose_extent(&capabilities, engine->window);

	free(formats);
	free(modes);

	Uint32 n_images = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && n_images > capabilities.maxImageCount)
		n_images = capabilities.maxImageCount;

	/* logging swapchain */
	A3D_LOG_INFO("creating vulkan swapchain");
	A3D_LOG_DEBUG("    images: %u", n_images);
	A3D_LOG_DEBUG("    format: %d", best_format.format);
	A3D_LOG_DEBUG("    present mode: %d", best_mode);
	A3D_LOG_DEBUG("    extent: %ux%u", extent.width, extent.height);

	/* creating swapchain */
	VkSwapchainCreateInfoKHR info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = n_images,
		.imageFormat = best_format.format,
		.imageColorSpace = best_format.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = best_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};
	
	Uint32 queue_indecies[] = {
		engine->vk.graphics_family,
		engine->vk.present_family
	};
	if (engine->vk.graphics_family != engine->vk.present_family) {
		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices = queue_indecies;
	}
	else {
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	result = vkCreateSwapchainKHR(engine->vk.logical, &info, NULL, &engine->vk.swapchain);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkCreateSwapchainKHR failed with code: %d", result);
		return false;
	}

	/* store chosen parameters */
	engine->vk.swapchain_format = best_format.format;
	engine->vk.swapchain_extent = extent;

	/* get swapchain images */
	vkGetSwapchainImagesKHR(engine->vk.logical, engine->vk.swapchain, &n_images, NULL);
	engine->vk.n_swapchain_images = n_images > 8 ? 8 : n_images;
	vkGetSwapchainImagesKHR(engine->vk.logical, engine->vk.swapchain, &engine->vk.n_swapchain_images, engine->vk.swapchain_images);

	A3D_LOG_INFO("created swapchain with %u images", n_images);
	return true;
}

bool a3d_vk_create_sync_objects(a3d* engine)
{
	A3D_LOG_INFO("creating sync objects");

	VkSemaphoreCreateInfo semaphore_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	if (vkCreateSemaphore(engine->vk.logical, &semaphore_info, NULL, &engine->vk.image_available) != VK_SUCCESS ||
		vkCreateSemaphore(engine->vk.logical, &semaphore_info, NULL, &engine->vk.render_finished) != VK_SUCCESS ||
		vkCreateFence(engine->vk.logical, &fence_info, NULL, &engine->vk.in_flight) != VK_SUCCESS) {

		A3D_LOG_ERROR("failed to create sync objects");
		return false;
	}
	A3D_LOG_INFO("created sync objects");
	return true;
}

void a3d_vk_destroy_command_pool(a3d* engine)
{
	if (engine->vk.command_pool) {
		vkDestroyCommandPool(engine->vk.logical, engine->vk.command_pool, NULL);
		engine->vk.command_pool = VK_NULL_HANDLE;
		A3D_LOG_INFO("command pool destroyed");
	}
}

void a3d_vk_destroy_framebuffers(a3d* engine)
{
	for (Uint32 i = 0; i < engine->vk.n_swapchain_images; i++) {
		if (engine->vk.framebuffers[i]) {
			vkDestroyFramebuffer(engine->vk.logical, engine->vk.framebuffers[i], NULL);
			engine->vk.framebuffers[i] = VK_NULL_HANDLE;
		}
	}
	A3D_LOG_INFO("destroyed framebuffers");
}

void a3d_vk_destroy_render_pass(a3d* engine)
{
	if (engine->vk.render_pass) {
		vkDestroyRenderPass(engine->vk.logical, engine->vk.render_pass, NULL);
		engine->vk.render_pass = VK_NULL_HANDLE;
		A3D_LOG_INFO("destroyed render pass");
	}
}

void a3d_vk_destroy_swapchain(a3d* engine)
{

	/* destroy image views */
	for (Uint32 i = 0; i < engine->vk.n_swapchain_images; i++) {
		if (engine->vk.swapchain_views[i]) {
			vkDestroyImageView(engine->vk.logical, engine->vk.swapchain_views[i], NULL);
			engine->vk.swapchain_views[i] = VK_NULL_HANDLE;
		}
	}
	A3D_LOG_INFO("vulkan destroyed image views");

	/* destroy swapchain */
	if (engine->vk.swapchain) {
		vkDestroySwapchainKHR(engine->vk.logical, engine->vk.swapchain, NULL);
		engine->vk.swapchain = VK_NULL_HANDLE;
		A3D_LOG_INFO("vulkan destroyed swapchain");
	}
}

void a3d_vk_destroy_sync_objects(a3d* engine)
{
	if (engine->vk.image_available)
		vkDestroySemaphore(engine->vk.logical, engine->vk.image_available, NULL);
	if (engine->vk.render_finished)
		vkDestroySemaphore(engine->vk.logical, engine->vk.render_finished, NULL);
	if (engine->vk.in_flight)
		vkDestroyFence(engine->vk.logical, engine->vk.in_flight, NULL);

	engine->vk.image_available = VK_NULL_HANDLE;
	engine->vk.render_finished = VK_NULL_HANDLE;
	engine->vk.in_flight = VK_NULL_HANDLE;

	A3D_LOG_INFO("sync objects destroyed");
}

bool a3d_vk_draw_frame(a3d* engine)
{
	vkWaitForFences(engine->vk.logical, 1, &engine->vk.in_flight, VK_TRUE, UINT64_MAX);
	vkResetFences(engine->vk.logical, 1, &engine->vk.in_flight);

	Uint32 image_index = 0;
	VkResult result = vkAcquireNextImageKHR(
		engine->vk.logical, engine->vk.swapchain, UINT64_MAX,
		engine->vk.image_available, VK_NULL_HANDLE, &image_index
	);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		A3D_LOG_WARN("vkAcquireNextImageKHR: swapchain out of date");
		a3d_vk_recreate_swapchain(engine);
		return false;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		A3D_LOG_ERROR("vkAcquireNextImageKHR failed with code %d", result);
		return false;
	}

	if (!a3d_vk_record_command_buffer(engine, image_index, engine->vk.clear_colour)) {
		A3D_LOG_ERROR("failed to re-record command buffer for image %u", image_index);
		return false;
	}

	/* submit recorded command buffer */
	VkSemaphore wait_semaphores[] = {engine->vk.image_available};
	VkSemaphore signal_semaphores[] = {engine->vk.render_finished};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	VkSubmitInfo submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = wait_semaphores,
		.pWaitDstStageMask = wait_stages,
		.commandBufferCount = 1,
		.pCommandBuffers = &engine->vk.command_buffers[image_index],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signal_semaphores
	};

	result = vkQueueSubmit(engine->vk.graphics_queue, 1, &submit, engine->vk.in_flight);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkQueueSubmit failed with code %d", result);
		return false;
	}

	/* present to screen */
	VkPresentInfoKHR present = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signal_semaphores,
		.swapchainCount = 1,
		.pSwapchains = &engine->vk.swapchain,
		.pImageIndices = &image_index
	};

	result = vkQueuePresentKHR(engine->vk.present_queue, &present);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result ==VK_SUBOPTIMAL_KHR) {
		A3D_LOG_WARN("swapchain needs recreation, present returned with code %d", result);
		a3d_vk_recreate_swapchain(engine);
	}
	else if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkQueuePresentKHR failed with code %d", result);
		return false;
	}

	return true;
}

bool a3d_vk_init(a3d* engine)
{
	A3D_LOG_INFO("initialising vulkan instance");

	Uint32 n_extensions = 0;
	const char* const* sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&n_extensions);
	if (!sdl_extensions) {
		A3D_LOG_ERROR("failed to retrieve vulkan extensions: %s", SDL_GetError());
		return false;
	}

	A3D_LOG_INFO("retrieved %u vulkan extensions from SDL", n_extensions);
	for (Uint32 i = 0; i < n_extensions; i++)
		A3D_LOG_DEBUG("\text[%u]: %s", i, sdl_extensions[i]);

	Uint32 n_enabled_extensions = n_extensions;
	const char* enabled_extensions[n_extensions + 1]; /* +1 for debugging */

	for (Uint32 i = 0; i < n_extensions; i++)
		enabled_extensions[i] = sdl_extensions[i];

#if A3D_VK_VALIDATION
	enabled_extensions[n_enabled_extensions++] = "VK_EXT_debug_utils";
	A3D_LOG_INFO("enabling VK_EXT_debug_utils for debug messenger");
#endif

	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,

		.pApplicationName = "Asimotive3D",
		.applicationVersion = VK_MAKE_VERSION(0, 1, 0),

		.pEngineName = "Asimotive3D",
		.engineVersion = VK_MAKE_VERSION(0, 1, 0),

		.apiVersion = VK_API_VERSION_1_3
	};

	VkInstanceCreateInfo instance_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = n_enabled_extensions,
		.ppEnabledExtensionNames = enabled_extensions,
#if A3D_VK_VALIDATION
		.enabledLayerCount = 1,
		.ppEnabledLayerNames = validation_layers
#else
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL
#endif
	};

	/* create instance */
	VkResult result = vkCreateInstance(&instance_info, NULL, &engine->vk.instance);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkCreateInstance failed with code: %d", result);
		engine->vk.instance = VK_NULL_HANDLE;
		return false;
	}
	A3D_LOG_INFO("vulkan instance created");

#if A3D_VK_VALIDATION
	a3d_vk_debug_init(engine);
#endif

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

	/* init swapchain & image views */
	if (!a3d_vk_create_swapchain(engine)) {
		A3D_LOG_ERROR("failed to create swapchain");
		return false;
	}

	if (!a3d_vk_create_image_views(engine)) {
		A3D_LOG_ERROR("failed to create image views");
		return false;
	}

	/* render pass */
	if (!a3d_vk_create_render_pass(engine)) {
		A3D_LOG_ERROR("failed to create render pass");
		return false;
	}

	/* graphics pipeline */
	if (!a3d_vk_create_graphics_pipeline(engine)) {
		A3D_LOG_ERROR("failed to create graphics pipeline");
		return false;
	}

	/* framebuffer */
	if (!a3d_vk_create_framebuffers(engine)) {
		A3D_LOG_ERROR("failed to create framebuffers");
		return false;
	}

	/* command pool */
	if (!a3d_vk_create_command_pool(engine)) {
		A3D_LOG_ERROR("failed to create command pool");
		return false;
	}

	if (!a3d_vk_allocate_command_buffers(engine)) {
		A3D_LOG_ERROR("failed to allocate command buffers");
		return false;
	}

	/* sync objects */
	if (!a3d_vk_create_sync_objects(engine)) {
		A3D_LOG_ERROR("failed to create sync objects");
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
	A3D_LOG_INFO("found %u devices", n_devices);

	VkPhysicalDevice devices[16];
	if (n_devices > 16) /* clamp */
		n_devices = 16;

	/* fill out devices */
	vkEnumeratePhysicalDevices(engine->vk.instance, &n_devices, devices);
	for (Uint32 i = 0; i < n_devices; i++) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(devices[i], &properties);

		const char* device_type = "UNKNOWN";

		switch (properties.deviceType) {
		case VK_PHYSICAL_DEVICE_TYPE_CPU: device_type = "CPU (Software Renderer)"; break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: device_type = "GPU (Discrete)"; break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: device_type = "GPU (Integrated)"; break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: device_type = "GPU (Virtual)"; break;
		case VK_PHYSICAL_DEVICE_TYPE_OTHER: device_type = "UNKNOWN (Other)"; break;
		/* remove warning */
		case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM: break;
		}

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

		a3d_vk_log_queue_families(engine, devices[i]);
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
		n_families = 32; /* clamp */

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

void a3d_vk_log_surface_support(a3d* engine)
{
	VkPhysicalDevice device = engine->vk.physical;

	VkSurfaceCapabilitiesKHR capabilities;
	VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		device, engine->vk.surface, &capabilities
	);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("failed to get surface capabilities");
		return;
	}

	/* log capabilities */
	A3D_LOG_DEBUG("min image count: %u", capabilities.minImageCount);
	A3D_LOG_DEBUG("max image count: %u", capabilities.maxImageCount);
	A3D_LOG_DEBUG("current extent: %ux%u", capabilities.currentExtent.width, capabilities.currentExtent.height);
	A3D_LOG_DEBUG("min extent: %ux%u", capabilities.minImageExtent.width, capabilities.minImageExtent.height);
	A3D_LOG_DEBUG("max extent: %ux%u", capabilities.maxImageExtent.width, capabilities.maxImageExtent.height);
	A3D_LOG_DEBUG("max image array layers: %u", capabilities.maxImageArrayLayers);
	A3D_LOG_DEBUG("supported transforms: 0x%x", capabilities.supportedTransforms);
	A3D_LOG_DEBUG("supported composite alpha: 0x%x", capabilities.supportedCompositeAlpha);
	A3D_LOG_DEBUG("supported usage flags: 0x%x", capabilities.supportedUsageFlags);

	/* surface formats */
	Uint32 n_formats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, engine->vk.surface, &n_formats, NULL);
	if (n_formats == 0) {
		A3D_LOG_ERROR("no formats available");
		return;
	}

	VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * n_formats);
	if (formats == NULL) {
		A3D_LOG_ERROR("ran out of memory! can't allocate memory for formats");
		return;
	}
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, engine->vk.surface, &n_formats, formats);
	
	A3D_LOG_INFO("found %u surface formats", n_formats);
	/* logging format properties */
	for (Uint32 i = 0; i < n_formats; i++) {
		A3D_LOG_DEBUG("    [%u] format=%d colourspace=%d", i, formats[i].format, formats[i].colorSpace);
	}
	free(formats);

	/* present modes */
	Uint32 n_modes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, engine->vk.surface, &n_modes, NULL);
	if (n_modes == 0) {
		A3D_LOG_ERROR("no present modes available");
		return;
	}

	VkPresentModeKHR* modes = malloc(sizeof(VkPresentModeKHR) * n_modes);
	if (modes == NULL) {
		A3D_LOG_ERROR("ran out of memory! can't allocate memory for modes");
		return;
	}
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, engine->vk.surface, &n_modes, modes);

	A3D_LOG_INFO("found %u present modes", n_modes);
	for (Uint32 i = 0; i < n_modes; i++) {
		const char* name = "UNKNOWN";
		switch (modes[i]) {
		case VK_PRESENT_MODE_IMMEDIATE_KHR: name = "IMMEDIATE"; break;
		case VK_PRESENT_MODE_MAILBOX_KHR: name = "MAILBOX"; break;
		case VK_PRESENT_MODE_FIFO_KHR: name = "FIFO (vsync)"; break;
		case VK_PRESENT_MODE_FIFO_RELAXED_KHR: name = "FIFO_RELAXED"; break;
		case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR: name = "SHARED_DEMAND"; break;
		case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR: name = "SHARED_CONTINUOUS"; break;
		/* remove warnings */
		case VK_PRESENT_MODE_FIFO_LATEST_READY_EXT: break;
		case VK_PRESENT_MODE_MAX_ENUM_KHR: break;
		}
		A3D_LOG_DEBUG("    [%u] %s (%d)", i, name, modes[i]);
		(void)name; /* remove warning */
	}
	free(modes);

	A3D_LOG_INFO("surface query complete");
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
	if (n_devices > 16) /* clamp */
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
		n_families = 32; /* clamp */
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

bool a3d_vk_record_command_buffers(a3d* engine)
{
	A3D_LOG_INFO("recording command buffers");

	for (Uint32 i = 0; i < engine->vk.n_swapchain_images; i++) {
		if (!a3d_vk_record_command_buffer(engine, i, engine->vk.clear_colour))
			return false;
	}

	A3D_LOG_INFO("recorded all command buffers");
	return true;
}

bool a3d_vk_record_command_buffer(a3d* engine, Uint32 i, VkClearValue clear)
{
	VkCommandBuffer* cmd = &engine->vk.command_buffers[i];
	vkResetCommandBuffer(*cmd, 0);

	VkCommandBufferBeginInfo buffer_begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};

	VkResult result = vkBeginCommandBuffer(engine->vk.command_buffers[i], &buffer_begin);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkBeginCommandBuffer failed with code %d", result);
		return false;
	}

	VkRenderPassBeginInfo render_begin = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = engine->vk.render_pass,
		.framebuffer = engine->vk.framebuffers[i],
		.renderArea = {
			.offset = {0, 0},
			.extent = engine->vk.swapchain_extent,
		},
		.clearValueCount = 1,
		.pClearValues = &clear
	};

	vkCmdBeginRenderPass(engine->vk.command_buffers[i], &render_begin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(*cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->vk.pipeline);
	const a3d_draw_item* items = NULL;
	Uint32 item_count = 0;
	a3d_renderer_get_draw_items(engine->renderer, &items, &item_count);
	for (Uint32 j = 0; j < item_count; j++) {
		const a3d_mesh* mesh = items[j].mesh;
		const a3d_mvp* mvp = &items[j].mvp;

		vkCmdPushConstants(*cmd, engine->vk.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(a3d_mvp), mvp);

		if (mesh && mvp)
			a3d_draw_mesh(engine, mesh, cmd);
	}

	vkCmdEndRenderPass(engine->vk.command_buffers[i]);

	result = vkEndCommandBuffer(engine->vk.command_buffers[i]);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkEndCommandBuffer failed with code %d", result);
		return false;
	}

	return true;
}

bool a3d_vk_recreate_swapchain(a3d* engine)
{
	int width = 0;
	int height = 0;
	SDL_GetWindowSize(engine->window, &width, &height);

	if (width == 0 || height == 0) { /* minimised */
		A3D_LOG_WARN("window minimised, skipping swapchain recreation");
		return false;
	}

	vkDeviceWaitIdle(engine->vk.logical);

	A3D_LOG_INFO("recreating swapchain with window %dx%d", width, height);

	/* destroy old objects */
	a3d_vk_destroy_framebuffers(engine);
	a3d_vk_destroy_graphics_pipeline(engine);
	a3d_vk_destroy_render_pass(engine);
	a3d_vk_destroy_swapchain(engine);

	/* recreate objects */
	if (!a3d_vk_create_swapchain(engine)) {
		A3D_LOG_ERROR("failed to recreate swapchain");
		return false;
	}

	if (!a3d_vk_create_image_views(engine)) {
		A3D_LOG_ERROR("failed to recreate image views");
		return false;
	}

	if (!a3d_vk_create_render_pass(engine)) {
		A3D_LOG_ERROR("failed to recreate render pass");
		return false;
	}

	if (!a3d_vk_create_graphics_pipeline(engine)) {
		A3D_LOG_ERROR("failed to recreate graphics pipeline");
		return false;
	}

	if (!a3d_vk_create_framebuffers(engine)) {
		A3D_LOG_ERROR("failed to recreate framebuffers");
		return false;
	}

	A3D_LOG_INFO("swapchain recreation complete");
	return true;
}

void a3d_vk_set_clear_colour(a3d* engine, float r, float g, float b, float a)
{
	engine->vk.clear_colour.color.float32[0] = r;
	engine->vk.clear_colour.color.float32[1] = g;
	engine->vk.clear_colour.color.float32[2] = b;
	engine->vk.clear_colour.color.float32[3] = a;
}

void a3d_vk_shutdown(a3d* engine)
{
	vkDeviceWaitIdle(engine->vk.logical);
	A3D_LOG_INFO("GPU finished work, destroying resources");

	a3d_vk_destroy_sync_objects(engine);
	a3d_vk_destroy_command_pool(engine);
	a3d_vk_destroy_graphics_pipeline(engine);
	a3d_vk_destroy_framebuffers(engine);
	a3d_vk_destroy_render_pass(engine);
	a3d_vk_destroy_swapchain(engine);

#if A3D_VK_VALIDATION
	a3d_vk_debug_shutdown(engine);
#endif

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

static VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR* capabilities, SDL_Window* window)
{
	if (capabilities->currentExtent.width != UINT32_MAX)
		return capabilities->currentExtent;

	int width;
	int height;
	SDL_GetWindowSize(window, &width, &height);
	VkExtent2D extent =  {
		.width = (Uint32)width,
		.height = (Uint32)height
	};

	/* clamp extents */
	if (extent.width  < capabilities->minImageExtent.width)
		extent.width  = capabilities->minImageExtent.width;
	if (extent.width  > capabilities->maxImageExtent.width)
		extent.width  = capabilities->maxImageExtent.width;
	if (extent.height < capabilities->minImageExtent.height)
		extent.height = capabilities->minImageExtent.height;
	if (extent.height > capabilities->maxImageExtent.height)
		extent.height = capabilities->maxImageExtent.height;

	return extent;
}

static VkPresentModeKHR choose_present_mode(const VkPresentModeKHR* modes, Uint32 n_modes)
{
	for (Uint32 i = 0; i < n_modes; i++) {
		if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			return modes[i];
	}
	return VK_PRESENT_MODE_FIFO_KHR; /* fallback & guaranteed */
}

static VkSurfaceFormatKHR choose_surface_format( const VkSurfaceFormatKHR* formats, Uint32 n_formats)
{
	for (Uint32 i = 0; i < n_formats; i++) {
		if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
		formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return formats[i];
	}
	return formats[0]; /* fallback */
}
