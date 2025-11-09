#pragma once

#include "a3d.h"

bool a3d_vk_allocate_command_buffers(a3d* engine);

bool a3d_vk_create_command_pool(a3d* engine);
bool a3d_vk_create_framebuffers(a3d* engine);
bool a3d_vk_create_image_views(a3d* engine);
bool a3d_vk_create_logical_device(a3d* engine);
bool a3d_vk_create_render_pass(a3d* engine);
bool a3d_vk_create_swapchain(a3d* engine);
bool a3d_vk_create_sync_objects(a3d* engine);

void a3d_vk_destroy_command_pool(a3d* engine);
void a3d_vk_destroy_framebuffers(a3d* engine);
void a3d_vk_destroy_render_pass(a3d* engine);
void a3d_vk_destroy_swapchain(a3d* engine);
void a3d_vk_destroy_sync_objects(a3d* engine);

bool a3d_vk_draw_frame(a3d* engine);

bool a3d_vk_init(a3d* engine);

void a3d_vk_log_devices(a3d* engine);
void a3d_vk_log_queue_families(a3d* engine, VkPhysicalDevice device);
void a3d_vk_log_surface_support(a3d* engine);

bool a3d_vk_pick_physical_device(a3d* engine);
bool a3d_vk_pick_queue_families(a3d* engine, VkPhysicalDevice device);

bool a3d_vk_record_command_buffers(a3d* engine);

bool a3d_vk_recreate_swapchain(a3d* engine);

void a3d_vk_shutdown(a3d* engine);
