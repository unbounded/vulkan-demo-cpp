#pragma once

// Renderable model with normals and texture coordinates
// Currently just used for the terrain

#include <vector>

#include <glm/glm.hpp>

#include "vulkan.hpp"


// Vertex layout for terrain model
struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
};


// Model with vertices and indices
struct Model {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};


// Drawable model with vertex and index buffers uploaded to device memory
struct UploadedModel {
	BufferAndMemory vertices;
	BufferAndMemory indices;
	size_t numIncides;

	// Upload model to device memory
	static UploadedModel fromModel(Model &model, VulkanState &vulkan);
};