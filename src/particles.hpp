#pragma once

#include <utility>

#include "vulkan.hpp"

std::pair<BufferAndMemory, size_t> makeParticles(VulkanState &vulkan);