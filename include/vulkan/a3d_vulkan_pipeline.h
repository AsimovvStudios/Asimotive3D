#pragma once

#include <vulkan/vulkan.h>

#include "a3d.h"

bool a3d_vk_create_graphics_pipeline(a3d* engine);
void a3d_vk_destroy_graphics_pipeline(a3d* engine);
