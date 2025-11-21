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

static VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR* caps, SDL_Window* window);
static VkSurfaceFormatKHR choose_surface_format( const VkSurfaceFormatKHR* fmts, Uint32 fmts_count);
static VkPresentModeKHR choose_present_mode(const VkPresentModeKHR* modes, Uint32 modes_count);

bool a3d_vk_allocate_command_buffers(a3d* e)
{
	A3D_LOG_INFO("allocating command buffers");

	VkCommandBufferAllocateInfo buff_alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = e->vk.cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = e->vk.swapchain_images_count
	};

	VkResult r = vkAllocateCommandBuffers(e->vk.logical, &buff_alloc_info, e->vk.cmd_buffs);
	if (r != VK_SUCCESS) {
		A3D_LOG_ERROR("failed to allocate command buffers with code %d", r);
		return false;
	}

	A3D_LOG_INFO("allocated %u command buffers", e->vk.swapchain_images_count);
	return true;
}

bool a3d_vk_create_command_pool(a3d* e)
{
	A3D_LOG_INFO("creating command pool");

	VkCommandPoolCreateInfo cmd_pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = e->vk.graphics_family,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
	};

	VkResult r = vkCreateCommandPool(e->vk.logical, &cmd_pool_info, NULL, &e->vk.cmd_pool);
	if (r != VK_SUCCESS) {
		A3D_LOG_ERROR("failed to create command pool with code %d", r);
		return false;
	}

	A3D_LOG_INFO("created command pool");
	return true;
}

bool a3d_vk_create_framebuffers(a3d* e)
{
	A3D_LOG_INFO("creating framebuffers");

	for (Uint32 i = 0; i < e->vk.swapchain_images_count; i++) {
		VkImageView attachments[] = {e->vk.swapchain_views[i]};

		VkFramebufferCreateInfo fb_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = e->vk.render_pass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = e->vk.swapchain_extent.width,
			.height = e->vk.swapchain_extent.height,
			.layers = 1
		};

		VkResult r = vkCreateFramebuffer(e->vk.logical, &fb_info, NULL, &e->vk.fbs[i]);
		if (r != VK_SUCCESS) {
			A3D_LOG_ERROR("failed to create framebuffer %u with code %d", i, r);
			return false;
		}
		A3D_LOG_INFO("created framebuffer %u", i);
	}

	return true;
}

bool a3d_vk_create_image_views(a3d* e)
{
	A3D_LOG_INFO("creating image views for swapchain");

	for (Uint32 i = 0; i < e->vk.swapchain_images_count; i++) {
		VkImageViewCreateInfo image_view_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = e->vk.swapchain_images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = e->vk.swapchain_fmt,
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

		VkResult r = vkCreateImageView(e->vk.logical, &image_view_info, NULL, &e->vk.swapchain_views[i]);
		if (r != VK_SUCCESS) {
			A3D_LOG_ERROR("failed to create image view %u with code %d", i, r);
			return false;
		}
		A3D_LOG_DEBUG("created image view %u", i);
	}
	A3D_LOG_INFO("all %u image views created", e->vk.swapchain_images_count);
	return true;
}

bool a3d_vk_create_logical_device(a3d* e)
{
	float priority = 1.0f;
	Uint32 unique_families[2] = {
		e->vk.graphics_family,
		e->vk.present_family
	};
	Uint32 unique_count = (e->vk.present_family == e->vk.graphics_family) ? 1 : 2;

	/* init queues info */
	VkDeviceQueueCreateInfo queues_info[2];
	for (Uint32 i = 0; i < unique_count; i++) {
		queues_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queues_info[i].pNext = NULL;
		queues_info[i].flags = 0;
		queues_info[i].queueFamilyIndex = unique_families[i];
		queues_info[i].queueCount = 1;
		queues_info[i].pQueuePriorities = &priority;
	}

	Uint32 device_extensions_count = 1;
	const char* device_extensions[] = {"VK_KHR_swapchain"};
	VkDeviceCreateInfo device_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = unique_count,
		.pQueueCreateInfos = queues_info,
		.enabledExtensionCount = device_extensions_count,
		.ppEnabledExtensionNames = device_extensions
	};

	VkResult r = vkCreateDevice(e->vk.physical, &device_info, NULL, &e->vk.logical);
	if (r != VK_SUCCESS) {
		A3D_LOG_ERROR("vkCreateDevice failed with code: %d", r);
		return false;
	}

	/* retrieve queue handles */
	vkGetDeviceQueue(e->vk.logical, e->vk.graphics_family, 0, &e->vk.graphics_queue);
	vkGetDeviceQueue(e->vk.logical, e->vk.present_family, 0, &e->vk.present_queue);

	A3D_LOG_INFO("logical device created");
	A3D_LOG_INFO("    graphics family: %u", e->vk.graphics_family);
	A3D_LOG_INFO("    present family: %u", e->vk.present_family);

	return true;
}

bool a3d_vk_create_render_pass(a3d* e)
{
	A3D_LOG_INFO("creating render pass");

	VkAttachmentDescription colour_attach = {
		.format = e->vk.swapchain_fmt,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference col_reference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &col_reference
	};

	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	};

	VkRenderPassCreateInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colour_attach,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	VkResult r = vkCreateRenderPass(e->vk.logical, &render_pass_info, NULL, &e->vk.render_pass);
	if (r != VK_SUCCESS) {
		A3D_LOG_ERROR("failed to create render pass with code %d", r);
		return false;
	}
	A3D_LOG_INFO("created render pass");
	return true;
}

bool a3d_vk_create_swapchain(a3d* e)
{
	VkPhysicalDevice device = e->vk.physical;
	VkSurfaceKHR surface = e->vk.surface;

	/* querying */
	VkSurfaceCapabilitiesKHR caps; /* capabilities */
	VkResult r = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &caps);
	if (r != VK_SUCCESS) {
		A3D_LOG_ERROR("failed to get surface capabilities with code %d", r);
		return false;
	}

	Uint32 fmts_count = 0; /* formats */
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &fmts_count, NULL);
	VkSurfaceFormatKHR* fmts = malloc(sizeof(VkSurfaceFormatKHR) * fmts_count);
	if (fmts == NULL) {
		A3D_LOG_ERROR("ran out of memory! can't allocate memory for formats");
		return false;
	}
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &fmts_count, fmts);

	Uint32 modes_count = 0; /* present modes */
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modes_count, NULL);
	VkPresentModeKHR* modes = malloc(sizeof(VkPresentModeKHR) * modes_count);
	if (modes == NULL) {
		A3D_LOG_ERROR("ran out of memory! can't allocate memory for modes");
		free(fmts);
		return false;
	}
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modes_count, modes);

	/* choose best */
	VkSurfaceFormatKHR best_format = choose_surface_format(fmts, fmts_count);
	VkPresentModeKHR best_mode = choose_present_mode(modes, modes_count);
	VkExtent2D extent = choose_extent(&caps, e->window);

	free(fmts);
	free(modes);

	Uint32 images_count = caps.minImageCount + 1;
	if (caps.maxImageCount > 0 && images_count > caps.maxImageCount)
		images_count = caps.maxImageCount;

	/* logging swapchain */
	A3D_LOG_INFO("creating vulkan swapchain");
	A3D_LOG_DEBUG("    images: %u", images_count);
	A3D_LOG_DEBUG("    format: %d", best_format.format);
	A3D_LOG_DEBUG("    present mode: %d", best_mode);
	A3D_LOG_DEBUG("    extent: %ux%u", extent.width, extent.height);

	/* creating swapchain */
	VkSwapchainCreateInfoKHR swapchain_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = images_count,
		.imageFormat = best_format.format,
		.imageColorSpace = best_format.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = caps.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = best_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};
	
	Uint32 queue_indecies[] = {
		e->vk.graphics_family,
		e->vk.present_family
	};
	if (e->vk.graphics_family != e->vk.present_family) {
		swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_info.queueFamilyIndexCount = 2;
		swapchain_info.pQueueFamilyIndices = queue_indecies;
	}
	else {
		swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	r = vkCreateSwapchainKHR(e->vk.logical, &swapchain_info, NULL, &e->vk.swapchain);
	if (r != VK_SUCCESS) {
		A3D_LOG_ERROR("vkCreateSwapchainKHR failed with code: %d", r);
		return false;
	}

	/* store chosen parameters */
	e->vk.swapchain_fmt = best_format.format;
	e->vk.swapchain_extent = extent;

	/* get swapchain images */
	vkGetSwapchainImagesKHR(e->vk.logical, e->vk.swapchain, &images_count, NULL);
	e->vk.swapchain_images_count = images_count > 8 ? 8 : images_count;
	vkGetSwapchainImagesKHR(e->vk.logical, e->vk.swapchain, &e->vk.swapchain_images_count, e->vk.swapchain_images);

	A3D_LOG_INFO("created swapchain with %u images", images_count);
	return true;
}

bool a3d_vk_create_sync_objects(a3d* e)
{
	A3D_LOG_INFO("creating sync objects");

	VkSemaphoreCreateInfo semaphore_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	if (vkCreateSemaphore(e->vk.logical, &semaphore_info, NULL, &e->vk.image_available) != VK_SUCCESS ||
		vkCreateSemaphore(e->vk.logical, &semaphore_info, NULL, &e->vk.render_finished) != VK_SUCCESS ||
		vkCreateFence(e->vk.logical, &fence_info, NULL, &e->vk.in_flight) != VK_SUCCESS) {

		A3D_LOG_ERROR("failed to create sync objects");
		return false;
	}
	A3D_LOG_INFO("created sync objects");
	return true;
}

void a3d_vk_destroy_command_pool(a3d* e)
{
	if (e->vk.cmd_pool) {
		vkDestroyCommandPool(e->vk.logical, e->vk.cmd_pool, NULL);
		e->vk.cmd_pool = VK_NULL_HANDLE;
		A3D_LOG_INFO("command pool destroyed");
	}
}

void a3d_vk_destroy_framebuffers(a3d* e)
{
	for (Uint32 i = 0; i < e->vk.swapchain_images_count; i++) {
		if (e->vk.fbs[i]) {
			vkDestroyFramebuffer(e->vk.logical, e->vk.fbs[i], NULL);
			e->vk.fbs[i] = VK_NULL_HANDLE;
		}
	}
	A3D_LOG_INFO("destroyed framebuffers");
}

void a3d_vk_destroy_render_pass(a3d* e)
{
	if (e->vk.render_pass) {
		vkDestroyRenderPass(e->vk.logical, e->vk.render_pass, NULL);
		e->vk.render_pass = VK_NULL_HANDLE;
		A3D_LOG_INFO("destroyed render pass");
	}
}

void a3d_vk_destroy_swapchain(a3d* e)
{

	/* destroy image views */
	for (Uint32 i = 0; i < e->vk.swapchain_images_count; i++) {
		if (e->vk.swapchain_views[i]) {
			vkDestroyImageView(e->vk.logical, e->vk.swapchain_views[i], NULL);
			e->vk.swapchain_views[i] = VK_NULL_HANDLE;
		}
	}
	A3D_LOG_INFO("vulkan destroyed image views");

	/* destroy swapchain */
	if (e->vk.swapchain) {
		vkDestroySwapchainKHR(e->vk.logical, e->vk.swapchain, NULL);
		e->vk.swapchain = VK_NULL_HANDLE;
		A3D_LOG_INFO("vulkan destroyed swapchain");
	}
}

void a3d_vk_destroy_sync_objects(a3d* e)
{
	if (e->vk.image_available)
		vkDestroySemaphore(e->vk.logical, e->vk.image_available, NULL);
	if (e->vk.render_finished)
		vkDestroySemaphore(e->vk.logical, e->vk.render_finished, NULL);
	if (e->vk.in_flight)
		vkDestroyFence(e->vk.logical, e->vk.in_flight, NULL);

	e->vk.image_available = VK_NULL_HANDLE;
	e->vk.render_finished = VK_NULL_HANDLE;
	e->vk.in_flight = VK_NULL_HANDLE;

	A3D_LOG_INFO("sync objects destroyed");
}

bool a3d_vk_draw_frame(a3d* e)
{
	vkWaitForFences(e->vk.logical, 1, &e->vk.in_flight, VK_TRUE, UINT64_MAX);
	vkResetFences(e->vk.logical, 1, &e->vk.in_flight);

	Uint32 image_index = 0;
	VkResult r = vkAcquireNextImageKHR(
		e->vk.logical, e->vk.swapchain, UINT64_MAX,
		e->vk.image_available, VK_NULL_HANDLE, &image_index
	);
	if (r == VK_ERROR_OUT_OF_DATE_KHR) {
		A3D_LOG_WARN("vkAcquireNextImageKHR: swapchain out of date");
		a3d_vk_recreate_swapchain(e);
		return false;
	}
	else if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
		A3D_LOG_ERROR("vkAcquireNextImageKHR failed with code %d", r);
		return false;
	}

	if (!a3d_vk_record_command_buffer(e, image_index, e->vk.clear_col)) {
		A3D_LOG_ERROR("failed to re-record command buffer for image %u", image_index);
		return false;
	}

	/* submit recorded command buffer */
	VkSemaphore wait_semaphores[] = {e->vk.image_available};
	VkSemaphore signal_semaphores[] = {e->vk.render_finished};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	VkSubmitInfo submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = wait_semaphores,
		.pWaitDstStageMask = wait_stages,
		.commandBufferCount = 1,
		.pCommandBuffers = &e->vk.cmd_buffs[image_index],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signal_semaphores
	};

	r = vkQueueSubmit(e->vk.graphics_queue, 1, &submit, e->vk.in_flight);
	if (r != VK_SUCCESS) {
		A3D_LOG_ERROR("vkQueueSubmit failed with code %d", r);
		return false;
	}

	/* present to screen */
	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signal_semaphores,
		.swapchainCount = 1,
		.pSwapchains = &e->vk.swapchain,
		.pImageIndices = &image_index
	};

	r = vkQueuePresentKHR(e->vk.present_queue, &present_info);
	if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) {
		A3D_LOG_WARN("swapchain needs recreation, present returned with code %d", r);
		a3d_vk_recreate_swapchain(e);
	}
	else if (r != VK_SUCCESS) {
		A3D_LOG_ERROR("vkQueuePresentKHR failed with code %d", r);
		return false;
	}

	return true;
}

bool a3d_vk_init(a3d* e)
{
	A3D_LOG_INFO("initialising vulkan instance");

	Uint32 extensions_count = 0;
	const char* const* sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&extensions_count);
	if (!sdl_extensions) {
		A3D_LOG_ERROR("failed to retrieve vulkan extensions: %s", SDL_GetError());
		return false;
	}

	A3D_LOG_INFO("retrieved %u vulkan extensions from SDL", extensions_count);
	for (Uint32 i = 0; i < extensions_count; i++)
		A3D_LOG_DEBUG("\text[%u]: %s", i, sdl_extensions[i]);

	Uint32 enabled_extensions_count = extensions_count;
	const char* enabled_extensions[extensions_count + 1]; /* +1 for debugging */

	for (Uint32 i = 0; i < extensions_count; i++)
		enabled_extensions[i] = sdl_extensions[i];

#if A3D_VK_VALIDATION
	enabled_extensions[enabled_extensions_count++] = "VK_EXT_debug_utils";
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
		.enabledExtensionCount = enabled_extensions_count,
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
	VkResult r = vkCreateInstance(&instance_info, NULL, &e->vk.instance);
	if (r != VK_SUCCESS) {
		A3D_LOG_ERROR("vkCreateInstance failed with code: %d", r);
		e->vk.instance = VK_NULL_HANDLE;
		return false;
	}

	A3D_LOG_INFO("vulkan instance created");

#if A3D_VK_VALIDATION
	a3d_vk_debug_init(e);
#endif

	/* create window surface */
	bool created_surface = SDL_Vulkan_CreateSurface(e->window, e->vk.instance, NULL, &e->vk.surface);

	if (!created_surface) {
		A3D_LOG_ERROR("failed to create surface: %s", SDL_GetError());
		vkDestroyInstance(e->vk.instance, NULL);
		e->vk.instance = VK_NULL_HANDLE;
		return false;
	}
	else {
		A3D_LOG_INFO("attached SDL vulkan surface");
	}

	/* init logical device */
	if (!a3d_vk_pick_physical_device(e)) {
		A3D_LOG_ERROR("failed to pick physical device");
		return false;
	}

	if (!a3d_vk_create_logical_device(e)) {
		A3D_LOG_ERROR("failed to create logical device");
		return false;
	}

	/* init swapchain & image views */
	if (!a3d_vk_create_swapchain(e)) {
		A3D_LOG_ERROR("failed to create swapchain");
		return false;
	}

	if (!a3d_vk_create_image_views(e)) {
		A3D_LOG_ERROR("failed to create image views");
		return false;
	}

	/* render pass */
	if (!a3d_vk_create_render_pass(e)) {
		A3D_LOG_ERROR("failed to create render pass");
		return false;
	}

	/* graphics pipeline */
	if (!a3d_vk_create_graphics_pipeline(e)) {
		A3D_LOG_ERROR("failed to create graphics pipeline");
		return false;
	}

	/* framebuffer */
	if (!a3d_vk_create_framebuffers(e)) {
		A3D_LOG_ERROR("failed to create framebuffers");
		return false;
	}

	/* command pool */
	if (!a3d_vk_create_command_pool(e)) {
		A3D_LOG_ERROR("failed to create command pool");
		return false;
	}

	if (!a3d_vk_allocate_command_buffers(e)) {
		A3D_LOG_ERROR("failed to allocate command buffers");
		return false;
	}

	/* sync objects */
	if (!a3d_vk_create_sync_objects(e)) {
		A3D_LOG_ERROR("failed to create sync objects");
		return false;
	}

	return true;
}

void a3d_vk_log_devices(a3d* e)
{
	Uint32 devices_count = 0;
	vkEnumeratePhysicalDevices(e->vk.instance, &devices_count, NULL);
	if (devices_count == 0) {
		A3D_LOG_ERROR("no vulkan-compatible GPU found!");
		return;
	}
	A3D_LOG_INFO("found %u devices", devices_count);

	VkPhysicalDevice devices[16];
	if (devices_count > 16) /* clamp */
		devices_count = 16;

	/* fill out devices */
	vkEnumeratePhysicalDevices(e->vk.instance, &devices_count, devices);
	for (Uint32 i = 0; i < devices_count; i++) {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(devices[i], &props);

		const char* device_type = "UNKNOWN";

		switch (props.deviceType) {
		case VK_PHYSICAL_DEVICE_TYPE_CPU: device_type = "CPU (Software Renderer)"; break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: device_type = "GPU (Discrete)"; break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: device_type = "GPU (Integrated)"; break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: device_type = "GPU (Virtual)"; break;
		case VK_PHYSICAL_DEVICE_TYPE_OTHER: device_type = "UNKNOWN (Other)"; break;
		/* remove warning */
		case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM: break;
		}

		/* logging */
		A3D_LOG_INFO("GPU[%u]: %s", i, props.deviceName);
		A3D_LOG_DEBUG("    type: %s", device_type);
		A3D_LOG_DEBUG(
			"    api version: %u.%u.%u",
			VK_API_VERSION_MAJOR(props.apiVersion),
			VK_API_VERSION_MINOR(props.apiVersion),
			VK_API_VERSION_PATCH(props.apiVersion)
		);
		A3D_LOG_DEBUG(
			"    driver version: %u.%u.%u",
			VK_API_VERSION_MAJOR(props.driverVersion),
			VK_API_VERSION_MINOR(props.driverVersion),
			VK_API_VERSION_PATCH(props.driverVersion)
		);
		(void)device_type; /* remove unused warning */

		a3d_vk_log_queue_families(e, devices[i]);
		a3d_vk_pick_queue_families(e, devices[i]);
	}
}

void a3d_vk_log_queue_families(a3d* e, VkPhysicalDevice device)
{
	/* query queue families */
	Uint32 families_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &families_count, NULL);
	if (families_count == 0) {
		A3D_LOG_ERROR("device has no queue families!");
		return;
	}

	VkQueueFamilyProperties families[32];
	if (families_count > 32)
		families_count = 32; /* clamp */

	vkGetPhysicalDeviceQueueFamilyProperties(device, &families_count, families);

	/* logging */
	A3D_LOG_INFO("found %u queue families", families_count);
	for (Uint32 i = 0; i < families_count; i++) {
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
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, e->vk.surface, &can_present);
		A3D_LOG_DEBUG("    presentation support: %s", can_present ? "YES" : "NO");
	}
}

void a3d_vk_log_surface_support(a3d* e)
{
	VkPhysicalDevice device = e->vk.physical;

	VkSurfaceCapabilitiesKHR caps;
	VkResult r = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		device, e->vk.surface, &caps
	);
	if (r != VK_SUCCESS) {
		A3D_LOG_ERROR("failed to get surface capabilities");
		return;
	}

	/* log capabilities */
	A3D_LOG_DEBUG("min image count: %u", caps.minImageCount);
	A3D_LOG_DEBUG("max image count: %u", caps.maxImageCount);
	A3D_LOG_DEBUG("current extent: %ux%u", caps.currentExtent.width, caps.currentExtent.height);
	A3D_LOG_DEBUG("min extent: %ux%u", caps.minImageExtent.width, caps.minImageExtent.height);
	A3D_LOG_DEBUG("max extent: %ux%u", caps.maxImageExtent.width, caps.maxImageExtent.height);
	A3D_LOG_DEBUG("max image array layers: %u", caps.maxImageArrayLayers);
	A3D_LOG_DEBUG("supported transforms: 0x%x", caps.supportedTransforms);
	A3D_LOG_DEBUG("supported composite alpha: 0x%x", caps.supportedCompositeAlpha);
	A3D_LOG_DEBUG("supported usage flags: 0x%x", caps.supportedUsageFlags);

	/* surface formats */
	Uint32 formats_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, e->vk.surface, &formats_count, NULL);
	if (formats_count == 0) {
		A3D_LOG_ERROR("no formats available");
		return;
	}

	VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * formats_count);
	if (formats == NULL) {
		A3D_LOG_ERROR("ran out of memory! can't allocate memory for formats");
		return;
	}
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, e->vk.surface, &formats_count, formats);
	
	A3D_LOG_INFO("found %u surface formats", formats_count);
	/* logging format properties */
	for (Uint32 i = 0; i < formats_count; i++) {
		A3D_LOG_DEBUG("    [%u] format=%d colourspace=%d", i, formats[i].format, formats[i].colorSpace);
	}
	free(formats);

	/* present modes */
	Uint32 modes_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, e->vk.surface, &modes_count, NULL);
	if (modes_count == 0) {
		A3D_LOG_ERROR("no present modes available");
		return;
	}

	VkPresentModeKHR* modes = malloc(sizeof(VkPresentModeKHR) * modes_count);
	if (modes == NULL) {
		A3D_LOG_ERROR("ran out of memory! can't allocate memory for modes");
		return;
	}
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, e->vk.surface, &modes_count, modes);

	A3D_LOG_INFO("found %u present modes", modes_count);
	for (Uint32 i = 0; i < modes_count; i++) {
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

bool a3d_vk_pick_physical_device(a3d* e)
{
	Uint32 devices_count = 0;
	vkEnumeratePhysicalDevices(e->vk.instance, &devices_count, NULL);
	if (devices_count == 0) {
		A3D_LOG_ERROR("no vulkan-compatible GPU found!");
		return false;
	}

	VkPhysicalDevice devices[16];
	if (devices_count > 16) /* clamp */
		devices_count = 16;
	vkEnumeratePhysicalDevices(e->vk.instance, &devices_count, devices);

	int best_score = -1;
	VkPhysicalDevice best = VK_NULL_HANDLE;
	for (Uint32 i = 0; i < devices_count; i++) {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(devices[i], &props);

		/* scoring */
		int score = 0;
		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 1000;
		else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			score += 500;
		else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
			score += 100;

		if (a3d_vk_pick_queue_families(e, devices[i]))
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

	e->vk.physical = best;

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(best, &props);
	A3D_LOG_INFO("selected GPU: %s", props.deviceName);

	return true;
}

bool a3d_vk_pick_queue_families(a3d* e, VkPhysicalDevice device)
{
	/* query queue families */
	Uint32 families_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &families_count, NULL);
	if (families_count == 0) {
		A3D_LOG_ERROR("device has no queue families!");
		return false;
	}

	VkQueueFamilyProperties families[32];
	if (families_count > 32)
		families_count = 32; /* clamp */
	vkGetPhysicalDeviceQueueFamilyProperties(device, &families_count, families);

	/* set sentinel */
	Uint32 graphics_family = UINT32_MAX;
	Uint32 present_family = UINT32_MAX;

	for (Uint32 i = 0; i < families_count; i++) {
		VkBool32 can_present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, e->vk.surface, &can_present);

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

	e->vk.graphics_family = graphics_family;
	e->vk.present_family = present_family;
	return true;
}

bool a3d_vk_record_command_buffers(a3d* e)
{
	A3D_LOG_INFO("recording command buffers");

	for (Uint32 i = 0; i < e->vk.swapchain_images_count; i++) {
		if (!a3d_vk_record_command_buffer(e, i, e->vk.clear_col))
			return false;
	}

	A3D_LOG_INFO("recorded all command buffers");
	return true;
}

bool a3d_vk_record_command_buffer(a3d* e, Uint32 i, VkClearValue clear)
{
	VkCommandBuffer* cmd = &e->vk.cmd_buffs[i];
	vkResetCommandBuffer(*cmd, 0);

	VkCommandBufferBeginInfo buffer_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};

	VkResult r = vkBeginCommandBuffer(e->vk.cmd_buffs[i], &buffer_begin_info);
	if (r != VK_SUCCESS) {
		A3D_LOG_ERROR("vkBeginCommandBuffer failed with code %d", r);
		return false;
	}

	VkRenderPassBeginInfo render_pass_begin_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = e->vk.render_pass,
		.framebuffer = e->vk.fbs[i],
		.renderArea = {
			.offset = {0, 0},
			.extent = e->vk.swapchain_extent,
		},
		.clearValueCount = 1,
		.pClearValues = &clear
	};

	vkCmdBeginRenderPass(e->vk.cmd_buffs[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(*cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, e->vk.pipeline);

	const a3d_draw_item* items = NULL;
	Uint32 item_count = 0;
	a3d_renderer_get_draw_items(e->renderer, &items, &item_count);
	for (Uint32 j = 0; j < item_count; j++) {
		const a3d_mesh* mesh = items[j].mesh;
		const a3d_mvp* mvp = &items[j].mvp;

		vkCmdPushConstants(*cmd, e->vk.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(a3d_mvp), mvp);

		if (mesh && mvp)
			a3d_draw_mesh(e, mesh, cmd);
	}

	vkCmdEndRenderPass(e->vk.cmd_buffs[i]);

	r = vkEndCommandBuffer(e->vk.cmd_buffs[i]);
	if (r != VK_SUCCESS) {
		A3D_LOG_ERROR("vkEndCommandBuffer failed with code %d", r);
		return false;
	}

	return true;
}

bool a3d_vk_recreate_swapchain(a3d* e)
{
	int width = 0;
	int height = 0;
	SDL_GetWindowSize(e->window, &width, &height);

	if (width == 0 || height == 0) { /* minimised */
		A3D_LOG_WARN("window minimised, skipping swapchain recreation");
		return false;
	}

	vkDeviceWaitIdle(e->vk.logical);

	A3D_LOG_INFO("recreating swapchain with window %dx%d", width, height);

	/* destroy old objects */
	a3d_vk_destroy_framebuffers(e);
	a3d_vk_destroy_graphics_pipeline(e);
	a3d_vk_destroy_render_pass(e);
	a3d_vk_destroy_swapchain(e);

	/* recreate objects */
	if (!a3d_vk_create_swapchain(e)) {
		A3D_LOG_ERROR("failed to recreate swapchain");
		return false;
	}

	if (!a3d_vk_create_image_views(e)) {
		A3D_LOG_ERROR("failed to recreate image views");
		return false;
	}

	if (!a3d_vk_create_render_pass(e)) {
		A3D_LOG_ERROR("failed to recreate render pass");
		return false;
	}

	if (!a3d_vk_create_graphics_pipeline(e)) {
		A3D_LOG_ERROR("failed to recreate graphics pipeline");
		return false;
	}

	if (!a3d_vk_create_framebuffers(e)) {
		A3D_LOG_ERROR("failed to recreate framebuffers");
		return false;
	}

	A3D_LOG_INFO("swapchain recreation complete");
	return true;
}

void a3d_vk_set_clear_colour(a3d* e, float r, float g, float b, float a)
{
	e->vk.clear_col.color.float32[0] = r;
	e->vk.clear_col.color.float32[1] = g;
	e->vk.clear_col.color.float32[2] = b;
	e->vk.clear_col.color.float32[3] = a;
}

void a3d_vk_shutdown(a3d* e)
{
	vkDeviceWaitIdle(e->vk.logical);
	A3D_LOG_INFO("GPU finished work, destroying resources");

	a3d_vk_destroy_sync_objects(e);
	a3d_vk_destroy_command_pool(e);
	a3d_vk_destroy_graphics_pipeline(e);
	a3d_vk_destroy_framebuffers(e);
	a3d_vk_destroy_render_pass(e);
	a3d_vk_destroy_swapchain(e);

#if A3D_VK_VALIDATION
	a3d_vk_debug_shutdown(e);
#endif

	if (e->vk.logical) {
		vkDestroyDevice(e->vk.logical, NULL);
		e->vk.logical = VK_NULL_HANDLE;
		A3D_LOG_INFO("vulkan logical device destroyed");
	}

	if (e->vk.surface) {
		vkDestroySurfaceKHR(e->vk.instance, e->vk.surface, NULL);
		e->vk.surface = VK_NULL_HANDLE;
		A3D_LOG_INFO("vulkan surface destroyed");
	}

	if (e->vk.instance) {
		vkDestroyInstance(e->vk.instance, NULL);
		e->vk.instance = VK_NULL_HANDLE;
		A3D_LOG_INFO("vulkan instance destroyed");
	}
}

static VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR* caps, SDL_Window* window)
{
	if (caps->currentExtent.width != UINT32_MAX)
		return caps->currentExtent;

	int width;
	int height;
	SDL_GetWindowSize(window, &width, &height);
	VkExtent2D extent =  {
		.width = (Uint32)width,
		.height = (Uint32)height
	};

	/* clamp extents */
	if (extent.width  < caps->minImageExtent.width)
		extent.width  = caps->minImageExtent.width;
	if (extent.width  > caps->maxImageExtent.width)
		extent.width  = caps->maxImageExtent.width;
	if (extent.height < caps->minImageExtent.height)
		extent.height = caps->minImageExtent.height;
	if (extent.height > caps->maxImageExtent.height)
		extent.height = caps->maxImageExtent.height;

	return extent;
}

static VkPresentModeKHR choose_present_mode(const VkPresentModeKHR* modes, Uint32 modes_count)
{
	for (Uint32 i = 0; i < modes_count; i++) {
		if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			return modes[i];
	}
	return VK_PRESENT_MODE_FIFO_KHR; /* fallback & guaranteed */
}

static VkSurfaceFormatKHR choose_surface_format( const VkSurfaceFormatKHR* fmts, Uint32 fmts_count)
{
	for (Uint32 i = 0; i < fmts_count; i++) {
		if (fmts[i].format == VK_FORMAT_B8G8R8A8_SRGB && fmts[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return fmts[i];
	}
	return fmts[0]; /* fallback */
}
