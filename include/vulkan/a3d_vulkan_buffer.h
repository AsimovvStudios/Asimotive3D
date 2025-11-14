#pragma once

#include <vulkan/vulkan.h>

#include "a3d.h"

typedef struct {
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize size;
} a3d_buffer;

bool a3d_vk_create_buffer(
	a3d* engine, VkDeviceSize size, VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties, a3d_buffer* out_buffer,
	const void* initial_data
);
void a3d_vk_destroy_buffer(a3d* engine, a3d_buffer* buffer);
