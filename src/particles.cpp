#include <vector>

#include "particles.hpp"

// Make a static buffer with particles
// Returns a buffer and the number of particles
std::pair<BufferAndMemory, size_t> makeParticles(VulkanState &vulkan) {
	// Just hardcode a few particles for now
	std::vector<Particle> particles {
		{
			{0.0, 0.0, 0.0},
			{0.0, 3.0, 0.1}
		},
		{
			{0.0, 0.0, 0.0},
			{0.1, 2.3, -0.1}
		},
		{
			{0.0, 0.5, 0.0},
			{-0.2, 2.4, -0.3}
		},
		{
			{0.0, 0.0, 0.0},
			{-0.02, 2.7, 0.22}
		}
	};

	return {
		vulkan.createBufferWithData(vk::BufferUsageFlagBits::eVertexBuffer, particles.size() * sizeof(Particle), reinterpret_cast<uint8_t*>(particles.data())),
		particles.size()
	};
}