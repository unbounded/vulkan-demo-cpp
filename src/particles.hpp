#pragma once

#include <utility>

#include "vulkan.hpp"


// Vertex layout for particles
struct Particle {
	glm::vec3 pos0;
	glm::vec3 v0;
};


std::pair<BufferAndMemory, size_t> makeParticles(VulkanState &vulkan);