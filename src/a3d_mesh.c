#include <SDL3/SDL_stdinc.h>
#include <vulkan/vulkan.h>

#include "a3d.h"
#include "a3d_logging.h"
#include "a3d_mesh.h"

void a3d_destroy_mesh(a3d* e, a3d_mesh* mesh)
{
	a3d_vk_destroy_buffer(e, &mesh->vertex_buffer);
	a3d_vk_destroy_buffer(e, &mesh->index_buffer);
	mesh->vertex_count = 0;
	mesh->index_count = 0;
	A3D_LOG_INFO("mesh destroyed");
}

void a3d_draw_mesh(a3d* engine, const a3d_mesh* mesh, VkCommandBuffer* cmd)
{
	(void) engine;
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(*cmd, 0, 1, &mesh->vertex_buffer.buff, offsets);
	vkCmdBindIndexBuffer(*cmd, mesh->index_buffer.buff, 0, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(*cmd, mesh->index_count, 1, 0, 0, 0);
}

bool a3d_init_triangle(a3d* e, a3d_mesh* mesh)
{
	A3D_LOG_INFO("creating triangle mesh");

	/* init */
	a3d_vertex vertices[] = {
		{{ 0.0f,  0.5f}, {1.0f, 0.0f, 0.0f}},
		{{-0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
		{{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	};

	Uint16 indices[] = {0, 1, 2};

	mesh->vertex_count = 3;
	mesh->index_count = 3;
	mesh->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	/* vertex buffer */
	bool r = a3d_vk_create_buffer(
		e, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&mesh->vertex_buffer, vertices
	);
	if (!r)
		return false;

	/* index buffer */
	r = a3d_vk_create_buffer(
		e, sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&mesh->index_buffer, indices
	);
	if (!r) {
		a3d_vk_destroy_buffer(e, &mesh->vertex_buffer);
		return false;
	}

	A3D_LOG_INFO("created triangle mesh");

	return true;
}
