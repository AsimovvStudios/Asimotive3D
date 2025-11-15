#include <string.h>
#include <vulkan/vulkan.h>

#include "a3d.h"
#include "a3d_logging.h"
#include "vulkan/a3d_vulkan_buffer.h"

static Uint32 find_memory_type(a3d* engine, Uint32 type_filter, VkMemoryPropertyFlags properties);

bool a3d_vk_create_buffer(
	a3d* engine, VkDeviceSize size, VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties, a3d_buffer* out_buffer,
	const void* initial_data
)
{
	A3D_LOG_INFO("creating buffer of %lu bytes", (Uint64)size);

	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VkResult result = vkCreateBuffer(engine->vk.logical, &buffer_info, NULL, &out_buffer->buffer);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkCreateBuffer failed with code %d", result);
		return false;
	}

	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(engine->vk.logical, out_buffer->buffer, &mem_requirements);

	/* check type index before passing */
	Uint32 type_index = find_memory_type(engine, mem_requirements.memoryTypeBits, properties);
	if (type_index == UINT32_MAX) {
		A3D_LOG_ERROR("no suitable memory type for buffer");
		vkDestroyBuffer(engine->vk.logical, out_buffer->buffer, NULL);
		out_buffer->buffer = VK_NULL_HANDLE;
		return false;
	}
	VkMemoryAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = mem_requirements.size,
		.memoryTypeIndex = type_index
	};
	
	result = vkAllocateMemory(engine->vk.logical, &allocate_info, NULL, &out_buffer->memory);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkAllocateMemory failed with code %d", result);
		return false;
	}

	/* upload initial data */
	if (initial_data) {
		void* data = NULL;
		vkMapMemory(engine->vk.logical, out_buffer->memory, 0, size, 0, &data);
		memcpy(data, initial_data, size);
		vkUnmapMemory(engine->vk.logical, out_buffer->memory);
	}


	vkBindBufferMemory(engine->vk.logical, out_buffer->buffer, out_buffer->memory, 0);

	out_buffer->size = size;
	A3D_LOG_INFO("created buffer");

	return true;
}

void a3d_vk_destroy_buffer(a3d* engine, a3d_buffer* buffer)
{
	if (buffer->buffer) {
		vkDestroyBuffer(engine->vk.logical, buffer->buffer, NULL);
		buffer->buffer = VK_NULL_HANDLE;
	}

	if (buffer->memory) {
		vkFreeMemory(engine->vk.logical, buffer->memory, NULL);
		buffer->memory = VK_NULL_HANDLE;
	}

	buffer->size = 0;
	A3D_LOG_INFO("destroyed buffer");
}

static Uint32 find_memory_type(a3d* engine, Uint32 type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(engine->vk.physical, &mem_properties);

	for (Uint32 i = 0; i < mem_properties.memoryTypeCount; i++) {
		if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}
	A3D_LOG_ERROR("failed to find suitable memory type");
	return UINT32_MAX;
}
