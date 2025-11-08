#pragma once

#include "a3d.h"

bool a3d_vk_create_logical_device(a3d* engine);
bool a3d_vk_create_swapchain(a3d* engine);
void a3d_vk_destroy_swapchain(a3d* engine);
bool a3d_vk_init(a3d* engine);
void a3d_vk_log_devices(a3d* engine);
void a3d_vk_log_queue_families(a3d* engine, VkPhysicalDevice device);
void a3d_vk_log_surface_support(a3d* engine);
bool a3d_vk_pick_physical_device(a3d* engine);
bool a3d_vk_pick_queue_families(a3d* engine, VkPhysicalDevice device);
void a3d_vk_shutdown(a3d* engine);
