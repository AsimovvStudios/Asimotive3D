#pragma once

#include "a3d.h"
#include "vulkan/a3d_vulkan_buffer.h"

typedef struct {
	float    position[2];
	float    colour[3];
} a3d_vertex;

typedef struct {
	a3d_buffer vertex_buffer;
	Uint32   n_vertex;

	a3d_buffer index_buffer;
	Uint32   n_index;

	VkPrimitiveTopology topology;
} a3d_mesh;

void a3d_destroy_mesh(a3d* engine, a3d_mesh* mesh);
void a3d_draw_mesh(a3d* engine, a3d_mesh* mesh, VkCommandBuffer cmd);
bool a3d_init_triangle(a3d* engine, a3d_mesh* mesh);
