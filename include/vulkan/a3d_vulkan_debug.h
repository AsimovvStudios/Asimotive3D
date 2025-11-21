#pragma once

#include <vulkan/vulkan.h>

#include "a3d.h"

#if A3D_VK_VALIDATION
bool a3d_vk_debug_init(a3d* e);
bool a3d_vk_debug_shutdown(a3d* e);
#endif
