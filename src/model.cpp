#include "model.hpp"


// Upload model to device memory
UploadedModel UploadedModel::fromModel(Model &model, VulkanState &vulkan) {
	auto vertices = vulkan.createBufferWithData(
		vk::BufferUsageFlagBits::eVertexBuffer,
		model.vertices.size() * sizeof(model.vertices[0]),
		reinterpret_cast<uint8_t*>(model.vertices.data())
	);
	auto indices = vulkan.createBufferWithData(
		vk::BufferUsageFlagBits::eIndexBuffer,
		model.indices.size() * sizeof(model.indices[0]),
		reinterpret_cast<uint8_t*>(model.indices.data())
	);
	return {
		std::move(vertices),
		std::move(indices),
		model.indices.size()
	};
}