#pragma once

#include "a3d.h"
#include <vulkan/vulkan_core.h>

bool a3d_vk_allocate_command_buffers(a3d* e);

bool a3d_vk_create_command_pool(a3d* e);
bool a3d_vk_create_depth_resources(a3d* e);
bool a3d_vk_create_framebuffers(a3d* e);
bool a3d_vk_create_image_views(a3d* e);
bool a3d_vk_create_logical_device(a3d* e);
bool a3d_vk_create_render_pass(a3d* e);
bool a3d_vk_create_swapchain(a3d* e);
bool a3d_vk_create_sync_objects(a3d* e);

void a3d_vk_destroy_command_pool(a3d* e);
void a3d_vk_destroy_depth_resources(a3d* e);
void a3d_vk_destroy_framebuffers(a3d* e);
void a3d_vk_destroy_render_pass(a3d* e);
void a3d_vk_destroy_swapchain(a3d* e);
void a3d_vk_destroy_sync_objects(a3d* e);

bool a3d_vk_draw_frame(a3d* e);

bool a3d_vk_init(a3d* e);

void a3d_vk_log_devices(a3d* e);
void a3d_vk_log_queue_families(a3d* e, VkPhysicalDevice device);
void a3d_vk_log_surface_support(a3d* e);

bool a3d_vk_pick_physical_device(a3d* e);
bool a3d_vk_pick_queue_families(a3d* e, VkPhysicalDevice device);

bool a3d_vk_record_command_buffers(a3d* e);
bool a3d_vk_record_command_buffer(a3d* e, Uint32 i, VkClearValue clear);

bool a3d_vk_recreate_swapchain(a3d* e);

void a3d_vk_set_clear_colour(a3d* e, float r, float g, float b, float a);

void a3d_vk_shutdown(a3d* e);
