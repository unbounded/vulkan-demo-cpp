
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "terrain.hpp"

// Dimensions of height map
const size_t MAP_SIZE = 32;


// Make a MAP_SIZE x MAP_SIZE height map with elevation between 0 and 1
static std::vector<float> generateHeightmap() {
	std::vector<float> map{};
	map.reserve(MAP_SIZE*MAP_SIZE);
	float half = MAP_SIZE / 2;
	for (size_t y = 0; y < MAP_SIZE; y++) {
		for (size_t x = 0; x < MAP_SIZE; x++) {
			float xc = (x - half) / half;
			float yc = (y - half) / half;
			float height = std::max(0.0f, sinf(std::min(3.14f, sqrtf((xc*xc) + (yc*yc)) * 6)));
			map.push_back(height);
		}
	}
	return map;
}


// Convert the height map to vertices with a calculated normal
static std::vector<Vertex> makeVertices(std::vector<float> &heights) {
	std::vector<Vertex> vertices{};
	// This is silly, should probably just store heights in proper structure
	auto lookup = [&](int x, int y) { return heights.at(y*MAP_SIZE + x);};
	const size_t lastRow = MAP_SIZE - 1;
	for (size_t y = 0; y < MAP_SIZE; y++) {
		for (size_t x = 0; x < MAP_SIZE; x++) {
			float height = lookup(x, y);
			float dx, dy;
			if (x == 0) {
				dx = (lookup(x+1, y) - lookup(x, y)) * 2;
			} else if (x == lastRow) {
				dx = (lookup(x, y) - lookup (x-1, y)) * 2;
			} else {
				dx = lookup(x+1, y) - lookup(x-1, y);
			}
			if (y == 0) {
				dy = (lookup(x, y+1) - lookup(x, y)) * 2;
			} else if (y == lastRow) {
				dy = (lookup(x, y) - lookup(x, y-1)) * 2;
			} else {
				dy = lookup(x, y+1) - lookup(x, y-1);
			}
			glm::vec3 pos {
				x / (float) MAP_SIZE - 0.5,
				height,
				y / (float) MAP_SIZE - 0.5,
			};
			glm::vec3 normal{ 2.0 * dx, -4.0, 2.0 * dy };
			glm::vec2 texCoord{ x / (float) MAP_SIZE, y / (float) MAP_SIZE};
			vertices.push_back({
				pos,
				glm::normalize(normal),
				texCoord
			});
		}
	}
	return vertices;
}


// Make indices to draw a solid mesh over the ground vertex
static std::vector<uint32_t> makeIndices() {
	std::vector<uint32_t> indices;
	for (size_t y = 0; y < MAP_SIZE - 1; y++) {
		for (size_t x = 0; x < MAP_SIZE - 1; x++) {
			// Make two triangles between each 2x2 pixel group
			uint32_t i = y * MAP_SIZE + x;
			indices.push_back(i);
			indices.push_back(i + 1);
			indices.push_back(i + MAP_SIZE);
			indices.push_back(i + 1);
			indices.push_back(i + 1 + MAP_SIZE);
			indices.push_back(i + MAP_SIZE);
		}
	}
	return indices;
}


// Make terrain model
Model makeTerrainModel() {
	auto heights = generateHeightmap();
	auto vertices = makeVertices(heights);
	auto indices = makeIndices();
	return {
		vertices,
		indices
	};
}