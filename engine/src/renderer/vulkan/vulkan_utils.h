#pragma once

#include "vulkan_defines.h"

// Result handling
const char* vulkan_result_string(VkResult result, bool extended);
bool vulkan_result_is_successful(VkResult result);