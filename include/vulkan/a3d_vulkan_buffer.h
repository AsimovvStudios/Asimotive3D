#pragma once

#include <vulkan/vulkan.h>

#include "a3d.h"

typedef struct {
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize size;
} a3d_buffer;

typedef struct {
	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_memory;
	Uint32   n_vertex;
} a3d_mesh;

bool a3d_vk_buffer_create(
	a3d* engine, VkDeviceSize size, VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties, a3d_buffer* out_buffer,
	const void* initial_data
);
void a3d_vk_buffer_destroy(a3d* engine, a3d_buffer* buffer);
